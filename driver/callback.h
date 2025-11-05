
#include "usbvideo.h"

#define BUF_STREAM_FRAME_READ	(1 << 2)
#define BUF_STREAM_READ			(1 << 1)
#define BUF_STREAM_EOF			(1 << 0)

// Structure de Buffer du Pilote
struct driver_buffer {
  struct completion	  new_frame_start;
  struct completion	  urb_completion;
  uint16_t  MaxLength;
  uint16_t  BytesUsed; 
  uint8_t   Status;
  int8_t 		LastFID;
  uint8_t  *Data;
};


static void complete_callback(struct urb *urb) {
	struct driver_buffer  *buffer = (struct driver_buffer *) urb->context;
	unsigned char *UrbPacketData;
	unsigned int   UrbPacketLength;
	unsigned int   MaxBufLength;
	unsigned int   nbytes;
	void 		  *BufData;
	int 		   i, ret;

	if (urb->status == 0) {
		for (i = 0; i < urb->number_of_packets; ++i) {
			if (urb->iso_frame_desc[i].status < 0) {
				continue;
			}
			
			UrbPacketData = urb->transfer_buffer + urb->iso_frame_desc[i].offset;
			if(UrbPacketData[1] & STREAM_ERR){
				continue;
			}
			
			UrbPacketLength = urb->iso_frame_desc[i].actual_length;
			if (UrbPacketLength < 2 || UrbPacketData[0] < 2 || UrbPacketData[0] > UrbPacketLength){
				continue;
			}

			if (buffer->LastFID != (UrbPacketData[1] & STREAM_FID)) {	// Détection du début d'un nouveau Frame
				buffer->Status &= ~BUF_STREAM_EOF;
				buffer->LastFID = (UrbPacketData[1] & STREAM_FID);
				if (buffer->Status & BUF_STREAM_READ) {
					buffer->Status |= BUF_STREAM_FRAME_READ;
					complete(&(buffer->new_frame_start));
				}
			}
			
			if (buffer->Status & BUF_STREAM_FRAME_READ) {	// Si en mode Read Frame, conserve les données reçues dans ce Paquet
				UrbPacketLength -= UrbPacketData[0];
				MaxBufLength = buffer->MaxLength - buffer->BytesUsed;
				BufData = buffer->Data + buffer->BytesUsed;
				nbytes = min(UrbPacketLength, MaxBufLength);
				memcpy(BufData, UrbPacketData + UrbPacketData[0], nbytes);
				buffer->BytesUsed += nbytes;
				if (UrbPacketLength > MaxBufLength) {
					break;
				}
			}

			if  (UrbPacketData[1] & STREAM_EOF) {	// Détection de la fin d'un Frame
				buffer->Status |= BUF_STREAM_EOF;
				break;
			}
		}

		if (buffer->Status & BUF_STREAM_FRAME_READ) {	// Tous les Paquets de ce Urb ont été reçus.
			if (buffer->Status & BUF_STREAM_EOF)
				buffer->Status &= ~BUF_STREAM_FRAME_READ;
			complete(&(buffer->urb_completion));
		}

		if ((ret = usb_submit_urb(urb, GFP_ATOMIC)) < 0) {	// Resoumet ce Urb pour continuer le Streaming
			printk(KERN_WARNING"ELE784 -> (%s) : Resubmit URB error =>  ret = %d\n", __FUNCTION__, ret);
		}

	} 
}



