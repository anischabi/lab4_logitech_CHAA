//from usb_video.h
#define STREAM_FID                  (1 << 0)
#define STREAM_EOF                  (1 << 1)
#define STREAM_ERR                  (1 << 6)

#define BUF_STREAM_FRAME_READ       (1 << 2)
#define BUF_STREAM_READ             (1 << 1)
#define BUF_STREAM_EOF              (1 << 0)

// Expected frame size for validation
#define EXPECTED_FRAME_SIZE         38400


// Structure de Buffer du Pilote
struct driver_buffer {
  struct completion   new_frame_start;
  struct completion   urb_completion;
  uint32_t    MaxLength;
  uint32_t    BytesUsed;
  uint8_t     Status;
  int8_t      LastFID;
  uint8_t    *Data;
};



static void complete_callback(struct urb *urb) {
    struct driver_buffer  *buffer = urb->context;
    unsigned char *UrbPacketData;
    unsigned int   UrbPacketLength;
    unsigned int   MaxBufLength;
    unsigned int   nbytes;
    void          *BufData;
    int            i, ret;
    uint8_t        currentFID;
    int            has_eof, has_fid_toggle;
    int            frame_complete;

    // Debug counters
    static int packet_count = 0;
    static int frame_count = 0;
    static int abandoned_count = 0;
    
    // Only process successful URBs or resubmit on recoverable errors
    if (urb->status != 0) {
        if (urb->status != -ENOENT && urb->status != -ECONNRESET && urb->status != -ESHUTDOWN) {
            ret = usb_submit_urb(urb, GFP_ATOMIC);
            if (ret < 0) {
                printk(KERN_WARNING "ELE784 -> (%s) : Resubmit URB error => ret = %d\n", __FUNCTION__, ret);
            }
        }
        return;
    }

    // Process all packets in this URB
    for (i = 0; i < urb->number_of_packets; ++i) {

        // Skip packets with errors
        if (urb->iso_frame_desc[i].status < 0)
            continue;

        UrbPacketData = urb->transfer_buffer + urb->iso_frame_desc[i].offset;
        UrbPacketLength = urb->iso_frame_desc[i].actual_length;
        
        // Validate packet has minimum header
        if (UrbPacketLength < 2 || UrbPacketData[0] < 2 || UrbPacketData[0] > UrbPacketLength)
            continue;

        // Skip packets with stream errors
        if (UrbPacketData[1] & STREAM_ERR)
            continue;

        currentFID = UrbPacketData[1] & STREAM_FID;
        has_eof = UrbPacketData[1] & STREAM_EOF;
        has_fid_toggle = (buffer->LastFID != currentFID);
        
        // Debug: Log important packets
        packet_count++;
        if (has_eof || has_fid_toggle || packet_count <= 50) {
            printk(KERN_INFO "ELE784 -> [Pkt %d] FID=%d LastFID=%d Toggle=%d EOF=%d Len=%u BytesUsed=%u Status=0x%02x\n",
                   packet_count,
                   currentFID,
                   buffer->LastFID,
                   has_fid_toggle,
                   has_eof,
                   UrbPacketLength,
                   buffer->BytesUsed,
                   buffer->Status);
        }

        // =====================================================
        // Handle packets with BOTH FID toggle AND EOF
        // These are frame boundary markers
        // =====================================================
        if (has_eof && has_fid_toggle) {
            printk(KERN_INFO "ELE784 -> [CASE 1] FID+EOF packet detected\n");
            
            // Copy packet data if actively capturing
            if (buffer->Status & BUF_STREAM_FRAME_READ) {
                UrbPacketLength -= UrbPacketData[0];
                MaxBufLength = buffer->MaxLength - buffer->BytesUsed;
                printk(KERN_INFO "ELE784 -> [CASE 1] Payload=%u, Space=%u\n", UrbPacketLength, MaxBufLength);
                
                if (MaxBufLength > 0) {
                    nbytes = min(UrbPacketLength, MaxBufLength);
                    BufData = buffer->Data + buffer->BytesUsed;
                    memcpy(BufData, UrbPacketData + UrbPacketData[0], nbytes);
                    buffer->BytesUsed += nbytes;
                    printk(KERN_INFO "ELE784 -> [CASE 1] Copied %u bytes, BytesUsed now %u\n", nbytes, buffer->BytesUsed);
                }
                
                // VALIDATE frame size before marking complete
                frame_complete = (buffer->BytesUsed >= EXPECTED_FRAME_SIZE);
                
                if (frame_complete) {
                    // Frame is complete - mark as done
                    buffer->Status |= BUF_STREAM_EOF;
                    buffer->Status &= ~BUF_STREAM_FRAME_READ;
                    
                    frame_count++;
                    printk(KERN_INFO "ELE784 -> [CASE 1] Frame #%d COMPLETE: %u bytes, Status=0x%02x\n",
                           frame_count, buffer->BytesUsed, buffer->Status);
                    complete(&(buffer->urb_completion));
                } else {
                    // Frame is NOT complete - ignore premature EOF
                    printk(KERN_WARNING "ELE784 -> [CASE 1] IGNORING premature EOF (FID+EOF): %u/%u bytes\n",
                           buffer->BytesUsed, EXPECTED_FRAME_SIZE);
                }
            }
            
            // Update LastFID regardless
            buffer->LastFID = currentFID;
            printk(KERN_INFO "ELE784 -> [CASE 1] Updated LastFID to %d\n", currentFID);
            
            // Skip further processing
            continue;
        }

        // =====================================================
        // Handle FID toggle (NEW frame detection)
        // CRITICAL: If we're currently capturing, abandon it!
        // =====================================================
        if (has_fid_toggle) {
            printk(KERN_INFO "ELE784 -> [CASE 2] FID toggle detected: %d -> %d\n", buffer->LastFID, currentFID);

            // If we were capturing a frame, abandon it (FID changed = new frame started)
            if (buffer->Status & BUF_STREAM_FRAME_READ) {
                abandoned_count++;
                printk(KERN_WARNING "ELE784 -> [CASE 2] ABANDONING incomplete frame: %u bytes (abandoned count: %d)\n",
                       buffer->BytesUsed, abandoned_count);
                
                // Clear the FRAME_READ flag to stop capturing the old frame
                buffer->Status &= ~BUF_STREAM_FRAME_READ;
            }

            buffer->LastFID = currentFID;
            
            // Only start NEW frame if ready and not already capturing
            if ((buffer->Status & BUF_STREAM_READ) && !(buffer->Status & BUF_STREAM_FRAME_READ)) {
                printk(KERN_INFO "ELE784 -> [CASE 2] Starting new frame\n");
                
                // Reset for new frame
                buffer->BytesUsed = 0;
                buffer->Status &= ~BUF_STREAM_EOF;
                buffer->Status |= BUF_STREAM_FRAME_READ;
                
                complete(&(buffer->new_frame_start));
                printk(KERN_INFO "ELE784 -> Frame START (FID=%d)\n", currentFID);
            } else {
                printk(KERN_INFO "ELE784 -> [CASE 2] NOT starting frame: READ=%d FRAME_READ=%d\n",
                       !!(buffer->Status & BUF_STREAM_READ),
                       !!(buffer->Status & BUF_STREAM_FRAME_READ));  
            }
        }

        // =====================================================
        // Copy payload data (only if actively capturing)
        // =====================================================
        if (buffer->Status & BUF_STREAM_FRAME_READ) {
            // Calculate payload size
            UrbPacketLength -= UrbPacketData[0];
            
            // Calculate available space
            MaxBufLength = buffer->MaxLength - buffer->BytesUsed;
            
            // Copy data if space available
            if (MaxBufLength > 0) {
                nbytes = min(UrbPacketLength, MaxBufLength);
                BufData = buffer->Data + buffer->BytesUsed;
                memcpy(BufData, UrbPacketData + UrbPacketData[0], nbytes);
                buffer->BytesUsed += nbytes;
           
                // Debug: Log data copy for first packets or EOF packets
                if (packet_count <= 50 || has_eof) {
                    printk(KERN_INFO "ELE784 -> [DATA] Copied %u bytes, BytesUsed now %u\n", nbytes, buffer->BytesUsed);
                }           
            } else if (packet_count <= 100) {
                printk(KERN_WARNING "ELE784 -> [DATA] Buffer full! BytesUsed=%u, MaxLength=%u\n", 
                       buffer->BytesUsed, buffer->MaxLength);
            }
        }

        // =====================================================
        // Handle EOF (without FID toggle)
        // CRITICAL: Validate frame size before accepting EOF
        // =====================================================
        if (has_eof && !has_fid_toggle) {
            printk(KERN_INFO "ELE784 -> [CASE 3] EOF without FID toggle detected\n");
            
            if (buffer->Status & BUF_STREAM_FRAME_READ) {
                // Check if frame is actually complete
                frame_complete = (buffer->BytesUsed >= EXPECTED_FRAME_SIZE);
                
                if (frame_complete) {
                    // Frame is complete - accept the EOF
                    buffer->Status |= BUF_STREAM_EOF;
                    buffer->Status &= ~BUF_STREAM_FRAME_READ;
                    
                    frame_count++;
                    printk(KERN_INFO "ELE784 -> [CASE 3] Frame #%d COMPLETE: %u bytes, Status=0x%02x\n",
                           frame_count, buffer->BytesUsed, buffer->Status);
                    complete(&(buffer->urb_completion));
                } else {
                    // Frame is NOT complete - ignore premature EOF
                    printk(KERN_WARNING "ELE784 -> [CASE 3] IGNORING premature EOF: %u/%u bytes (continuing capture)\n",
                           buffer->BytesUsed, EXPECTED_FRAME_SIZE);
                }
            } else {
                printk(KERN_INFO "ELE784 -> [CASE 3] EOF ignored (not capturing), Status=0x%02x\n", buffer->Status);
            }
        }
    }

    // Re-submit URB for continuous streaming
    ret = usb_submit_urb(urb, GFP_ATOMIC);
    if (ret < 0) {
        printk(KERN_WARNING "ELE784 -> URB resubmit failed: %d\n", ret);
    }
}
