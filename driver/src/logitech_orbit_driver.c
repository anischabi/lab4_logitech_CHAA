#include "logitech_orbit_driver.h"



// Event when the device is opened (Called when user-space opens /dev/camera_control or c.)
int ele784_open(struct inode *inode, struct file *file) {
  struct usb_interface *interface;
  int subminor;
  
  printk(KERN_WARNING "ELE784 -> Open\n");
  //gets the minor number assigned to /dev/camera_streamX or /dev/camera_controlX
  subminor = iminor(inode);
  
  // Retrieves the USB interface corresponding to the minor number of the device.
  // finds which USB interface corresponds to this minor number
  // (udriver comes from .h file)
  interface = usb_find_interface(&udriver, subminor);
  if (!interface) {
    //If not found, device not found : return -ENODEV.
    printk(KERN_WARNING "ELE784 -> Open: Ne peux ouvrir le peripherique\n");
    return -ENODEV;
  }

  //Stores the per-device private data (orbit_driver struct) in file->private_data.
  // This means every call to read, ioctl, etc. can retrieve the device data 
  file->private_data = usb_get_intfdata(interface);

  return 0;
}

// Probing USB device
int ele784_probe(struct usb_interface *interface, const struct usb_device_id *id) {
  struct orbit_driver *dev;
  struct usb_host_interface *iface_desc;
  int i,j;
  int retval;

  printk(KERN_INFO "ELE784 -> Probe: device connected\n");
  dev = kzalloc(sizeof(*dev), GFP_KERNEL);
  if (!dev) {
    dev_err(&interface->dev, "ELE784 -> Probe : Out of memory\n");
    return -ENOMEM;
  }

  /* Save interface + device pointer */
  dev->device = usb_get_dev(interface_to_usbdev(interface));
  dev->interface = interface;

  // Initialize URB pointers to NULL.
  for (i = 0; i < URB_COUNT; ++i)
    dev->isoc_in_urb[i] = NULL;

  // Get interface descriptor : Determine what kind of interface this is so we know whether to register camera_control or camera_stream.
  iface_desc = interface->cur_altsetting;

  // printk(KERN_INFO "ELE784 -> Probe interface number: %d\n", iface_desc->desc.bInterfaceNumber);
  // printk(KERN_INFO "ELE784 -> Interface class: 0x%02x, subclass: 0x%02x, protocol: 0x%02x\n",
  //  iface_desc->desc.bInterfaceClass,
  //  iface_desc->desc.bInterfaceSubClass,
  //  iface_desc->desc.bInterfaceProtocol);

  // Print all alternate settings and endpoint info for this interface
  for (i = 0; i < interface->num_altsetting; i++) {
    struct usb_host_interface *alts = &interface->altsetting[i];
    // printk(KERN_INFO "ELE784 -> AltSetting %d: bNumEndpoints=%d\n",alts->desc.bAlternateSetting, alts->desc.bNumEndpoints);
    for (j = 0; j < alts->desc.bNumEndpoints; j++) {
      struct usb_endpoint_descriptor *epd = &alts->endpoint[j].desc;
      // printk(KERN_INFO "ELE784 ->  Endpoint %d: addr=0x%02x, attr=0x%02x, maxpacket=%d\n",j, epd->bEndpointAddress, epd->bmAttributes, epd->wMaxPacketSize);
    }
  }
  
 /* 2.
  * Check if this interface belongs to the Video class.
  * USB class codes: CC_VIDEO = 0x0E.
  * This ensures we only handle interfaces for webcams, not other USB devices.
  */
  if (iface_desc->desc.bInterfaceClass == CC_VIDEO) {
    /* 2.A.
     * Save our driver’s private data (orbit_driver struct (dev)) in the interface.
     * This allows other functions (open, read, ioctl, etc.) to retrieve
     * the driver state for this specific interface using:
     *      file->private_data = usb_get_intfdata(interface);
     */
    usb_set_intfdata(interface, dev);
    /* 2.B.
     * Check if this is the VIDEO CONTROL interface.
     * Video Control (bInterfaceSubClass = 0x01) handles commands like:
     *  - Pan/Tilt
     *  - Zoom
     *  - Brightness/contrast
     * This interface is used for **camera settings and control**.
     */
    if (iface_desc->desc.bInterfaceSubClass == SC_VIDEOCONTROL) {
      /* 2.B.2.
        * Assign the class driver responsible for control commands.
        * This will create the device node: /dev/camera_control
        */
      dev->class_driver = &class_control_driver;
      /* 2.B.1.
        * Register the device with the USB core so that a device node is created for control.
        * usb_register_dev() connects the kernel USB interface to the character device.
        */
      retval = usb_register_dev(interface, &class_control_driver);
      if (retval) {
        /* 
        * Registration failed — cleanup:
        * 1. Remove the reference from the interface
        * 2. Free the driver struct memory
        * 3. Return the error code
        */
        printk(KERN_ERR "ELE784 -> Probe : Could not register camera_control\n");
        usb_set_intfdata(interface, NULL);
        kfree(dev);
        dev = NULL;
        return retval;
      }
      printk(KERN_INFO "ELE784 -> Probe : Registered camera_control device\n");
      return 0; // success
    }
    /* 2.C.
     * Check if this is the VIDEO STREAMING interface.
     * Video Streaming (bInterfaceSubClass = 0x02) handles the actual
     * video data streaming from the webcam via isochronous endpoints.
     * This interface will create /dev/camera_stream for user-space access.
     */
    else if(iface_desc->desc.bInterfaceSubClass == SC_VIDEOSTREAMING){
      /* 2.C.2.
       * Assign the class driver for video streaming .
       * This will create the device node: /dev/camera_stream 
       */
      dev->class_driver = &class_stream_driver;
      /* 2.C.1. 
       *Register the device node for streaming 
       */
      retval = usb_register_dev(interface, &class_stream_driver);
      if (retval) {
        /*
         * Registration failed — cleanup:
         * 1. Remove the interface driver data
         * 2. Free memory
         * 3. Return error
         */
        printk(KERN_ERR "ELE784 -> Probe : Could not register camera_stream\n");
        usb_set_intfdata(interface, NULL);
        kfree(dev);
        dev = NULL;
        return retval;
      }
      printk(KERN_INFO "ELE784 -> Probe : Registered camera_stream device\n");
      return 0; // success
    }
    /* 2.D. Video class but unknown subclass */
    printk(KERN_INFO "ELE784 -> Probe : Video interface but unknown subclass\n");
    usb_set_intfdata(interface, NULL);
    kfree(dev);
    dev = NULL;
    return -ENODEV;
  } 
  /* Not a video interface */
  kfree(dev);
  dev = NULL;
  return -ENODEV;
}

// This is the disconnect callback called by the USB core when the device is physically unplugged or the driver is removed.
// intf is the USB interface being disconnected.
void ele784_disconnect(struct usb_interface *intf) {
  // i is just a loop counter for later.
  int i; 
  // 1. usb_get_intfdata(intf) retrieves the pointer to driver’s private data (struct orbit_driver) 
  // that previously attached in probe() with usb_set_intfdata().
  struct orbit_driver *dev = usb_get_intfdata(intf); 
  printk(KERN_INFO "ELE784 -> Disconnect\n");
  // 2. If the private driver data wasn’t set (somehow probe() never succeeded), there’s nothing to clean up.
  if (!dev)
    return;
  /* 2.A. Deregister the device node (camera_control or camera_stream).
   * - Unregisters the character device node created by usb_register_dev() in probe().
   * - Removes /dev/camera_control or /dev/camera_stream from the system before freeing any memory.
   */
  usb_deregister_dev(intf, dev->class_driver);
  /* 2.B. If URBs were allocated (streaming started) 
   * - Checks whether any isochronous URBs were allocated. 
   * - If the user hasn’t started streaming yet, we don’t need to touch URBs or buffers.
   */
  if (dev->isoc_in_urb[0] != NULL) 
  {
    //Loop over all URBs in the driver.
    for (i = 0; i < URB_COUNT; i++) 
    {
      //dev->isoc_in_urb[i] may be NULL if that URB was never allocated. (Only free URBs that exist.)
      if (dev->isoc_in_urb[i]) 
      {
        /* 2.B.1. Stop URB if running.
         * - Cancels the URB if it’s still pending (being processed by the USB core).
         * - Prevents the kernel from accessing freed memory.
         */
        usb_kill_urb(dev->isoc_in_urb[i]);
        /* 2.B.2. Free DMA buffer 
         * - Frees the DMA buffer associated with the URB.
         * - usb_free_coherent() releases the memory allocated by usb_alloc_coherent()
         * - dev->frame_buf.MaxLength is the size of the buffer
         * - Avoid memory leaks and ensure proper cleanup of USB DMA resources.
         */
        if (dev->isoc_in_urb[i]->transfer_buffer)
        {
          usb_free_coherent(dev->device,
                            dev->frame_buf.MaxLength,
                            dev->isoc_in_urb[i]->transfer_buffer,
                            dev->isoc_in_urb[i]->transfer_dma);
        }
        /* 2.B.3. Frees the memory of the URB structure itself.
         * - Sets the pointer to NULL to avoid dangling pointers.
         * - Complete cleanup of all allocated URB resources.
         */
        usb_free_urb(dev->isoc_in_urb[i]);
        dev->isoc_in_urb[i] = NULL;
      }
    }
    /* 2.B.4. Free the frame buffer.
     * - Frees the frame buffer used to store video frames collected from URBs.
     * - Sets pointer to NULL to avoid dangling pointer.
     * - Prevent memory leaks and ensure the driver struct is completely cleaned.
     */
    if (dev->frame_buf.Data) {
      kfree(dev->frame_buf.Data);
      dev->frame_buf.Data = NULL;
    }
  }
  // **Null out file->private_data for all open fds**
  // You must iterate over all open files referencing this device.
  // Simplest workaround: store drv->file_list or just set drv->interface = NULL
  dev->interface = NULL;
  /* 2.C. Detach driver data from interface 
   *    - Ensures that if open() or other calls happen after disconnect, they won’t accidentally use freed memory.
   */
  usb_set_intfdata(intf, NULL);
  /* 2.D. Free the driver struct 
   *    - Last step in cleanup; all sub-structures have already been freed.
   */
  kfree(dev);
  dev = NULL;
  printk(KERN_INFO "ELE784 -> Disconnect complete\n");
}

// IOCTL handler for camera control commands
long ele784_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {

  // Retrieve the driver data from file->private_data
  struct orbit_driver  *driver = (struct orbit_driver *) file->private_data;
  
  // Validate the driver data
  if (!driver) {
    // If private_data is NULL, the device is disconnected or not properly opened
    printk(KERN_ERR "ELE784 -> IOCTL called on disconnected device (private_data=NULL)\n");
    return -ENODEV;
  }else{
    // Log the received IOCTL command for debugging
    printk(KERN_INFO "ELE784 -> IOCTL called with cmd=0x%08x\n", cmd);
  }

  // Further validate the interface pointer
  if (!driver->interface) {
    // If interface is NULL, the device has been disconnected
    printk(KERN_ERR "ELE784 -> IOCTL called on disconnected device (interface=NULL)\n");
    return -ENODEV;
  }else{
    // Log that the interface is valid
    printk(KERN_INFO "ELE784 -> IOCTL device interface is valid\n");
  }

  //device and interface pointers
  struct usb_interface *interface = driver->interface;
  // Get the usb_device structure for sending control messages
  struct usb_device *udev = interface_to_usbdev(interface);

  // Local variables for USB request parameters
  struct usb_request user_request; // structure to hold user request
  uint8_t  request, data_size; // size of data buffer
  uint16_t value, index, timeout; // USB request parameters
  uint8_t  *data = NULL; // data buffer pointer

  int i,j; // loop counters
  long retval=0; // return value

  // Handle different IOCTL commands
  switch(cmd) {

    // Handle IOCTL_GET command (get = reads from device/ ask to device)
    case IOCTL_GET:
      printk(KERN_INFO "ELE784 -> IOCTL_GET\n");
      /* ---------------------------------------------------------
      * Step 1 — Copy the usb_request header from user space
      * --------------------------------------------------------- */
      if (copy_from_user(&user_request, (struct usb_request __user *)arg, sizeof(struct usb_request))) {
        printk(KERN_ERR "ELE784 -> IOCTL_GET : copy_from_user (request) failed\n");
        retval = -EFAULT;
        break;
      }
      
      /* Basic sanity check */      
      data_size = user_request.data_size;
      if (data_size < 0 || data_size > 1024) {
          printk(KERN_ERR "ELE784 -> IOCTL_GET: invalid data_size=%d [0;1024]\n", data_size);
          retval = -EINVAL;
          break;
      }
      /* ---------------------------------------------------------
      * Step 2 — Allocate kernel buffer for receiving USB payload
      * --------------------------------------------------------- */
      if (data_size > 0) {
        data = kmalloc(data_size, GFP_KERNEL);
        if (!data) {
          printk(KERN_ERR "ELE784 -> IOCTL_GET: kmalloc(%d) failed\n",data_size);
          retval = -ENOMEM;
          break;
        }
      }
      /* ---------------------------------------------------------
      * Step 3 — Perform the USB GET request (device → host)
      * --------------------------------------------------------- */
      retval = usb_control_msg(
          udev,
          usb_rcvctrlpipe(udev, 0), // Control-IN : endpoint 0
          user_request.request,   // GET_CUR, GET_MIN, GET_MAX, ...
          USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE,
          user_request.value,     // full 16-bit wValue already provided by user
          user_request.index,     // full 16-bit wIndex (interface) already provided
          data,                   // destination buffer
          data_size,              // expected length
          user_request.timeout
      );

      /* ---------------------------------------------------------
      * Step 4 — Handle USB errors
      * --------------------------------------------------------- */
      if(retval < 0) {
        /* USB failure error message already contained in retval */
        printk(KERN_ERR "ELE784 -> IOCTL_GET: usb_control_msg failed (%d)\n",retval);
        if (data) {
          kfree(data);
          data = NULL;
        }
        break; // to freeing region at bottom       
      }
      /* ---------------------------------------------------------
      * Step 5 — Warn if short packet (should never happen for UVC)
      * --------------------------------------------------------- */
      if (retval != data_size) {
        /* Continue anyway, return what we received */
        printk(KERN_WARNING "ELE784 -> IOCTL_GET: short packet (%d/%d bytes)\n",retval, data_size);
      }
      /* ---------------------------------------------------------
      * Step 6 — Copy data back to user space
      * --------------------------------------------------------- */
      if (data_size > 0) {
        if (copy_to_user(user_request.data, data, data_size)) {
          printk(KERN_ERR "ELE784 -> IOCTL_GET: copy_to_user failed\n");
          if (data) {
            kfree(data);
            data = NULL;
          }
          retval = -EFAULT;
          break;
        }
      }
      /* api de ioctl demande 0 comme retour en cas de succes 
      * Success: use 0 as ioctl return code 
      */
      retval = 0;
      /* ---------------------------------------------------------
      * Step 7 — Free kernel buffer
      * --------------------------------------------------------- */
      if (data) {
        kfree(data);
        data = NULL;
      }
      break; //  Required to exit the switch

    // set = write a value to device. 
    case IOCTL_SET:
        printk(KERN_INFO "ELE784 -> IOCTL_SET\n");
        /* ---------------------------------------------------------
        * Step 1 — Copy the usb_request header from user space
        * --------------------------------------------------------- */
        if (copy_from_user(&user_request,(struct usb_request __user *)arg,sizeof(struct usb_request))) {
            printk(KERN_ERR "ELE784 -> IOCTL_SET: copy_from_user(request) failed\n");
            retval = -EFAULT;
            break;
        }
        /* Basic sanity check */
        data_size = user_request.data_size;
        if (data_size < 0 || data_size > 1024) {
          printk(KERN_ERR "ELE784 -> IOCTL_SET: invalid data_size=%d [0;1024]\n",data_size);
          retval = -EINVAL;
          break;
        }
        /* ---------------------------------------------------------
        * Step 2 — Allocate kernel buffer for sending USB payload
        * --------------------------------------------------------- */
        if (data_size > 0) {
          data = kmalloc(data_size, GFP_KERNEL);
          if (!data) {
            printk(KERN_ERR "ELE784 -> IOCTL_SET: kmalloc(%d) failed\n",data_size);
            retval = -ENOMEM;
            break;
          }
          /* Copy payload (host → device) */
          if (copy_from_user(data,(uint8_t __user *)user_request.data,data_size)) {
            printk(KERN_ERR "ELE784 -> IOCTL_SET: copy_from_user(data) failed\n");
            if (data) {
              kfree(data);
              data = NULL;
            }
            retval = -EFAULT;
            break;
          }
        }
        /* ---------------------------------------------------------
        * Step 3 — Perform the USB SET request (host → device)
        * --------------------------------------------------------- */
        retval = usb_control_msg(
            udev,
            usb_sndctrlpipe(udev, 0),
            user_request.request,   // Typically SET_CUR = 1
            USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE,
            user_request.value,     // full 16-bit wValue
            user_request.index,     // full 16-bit wIndex
            data,
            data_size,
            user_request.timeout
        );
        /* ---------------------------------------------------------
        * Step 4 — Handle USB errors
        * --------------------------------------------------------- */
        if (retval < 0) {
          printk(KERN_ERR "ELE784 -> IOCTL_SET: usb_control_msg failed (%d)\n",retval);
          if (data) {
            kfree(data);
            data = NULL;
          }
          break; // exit this case
        }
        /* On success, ioctl returns 0 */
        retval = 0;
        /* ---------------------------------------------------------
        * Step 5 — Free kernel buffer
        * --------------------------------------------------------- */
        if (data) {
          kfree(data);
          data = NULL;
        }
        break;

  
    case IOCTL_PANTILT_RESET:
      printk(KERN_INFO "ELE784 -> IOCTL_PANTILT_RESET\n");
      /* ---------------------------------------------------------
      * Step 1 — Allocate kernel buffer (1 byte for reset command)
      * --------------------------------------------------------- */
      data = kmalloc(1, GFP_KERNEL);
      if (!data) {
        printk(KERN_ERR "ELE784 -> IOCTL_PANTILT_RESET: kmalloc(1) failed\n");
        retval = -ENOMEM;
        break;
      }
      /* ---------------------------------------------------------
      * Step 2 — Fill payload
      * --------------------------------------------------------- */
      data[0] = PANTILT_RESET_CMD;
      /* ---------------------------------------------------------
      * Step 3 — Perform USB SET request (host → device)
      * --------------------------------------------------------- */
      retval = usb_control_msg(udev,
                              usb_sndctrlpipe(udev, 0),
                              SET_CUR,
                              USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE,
                              PANTILT_RESET_CONTROL,
                              PANTILT_INDEX, // wIndex interface : 00 and entity : 0B (0X0B00)
                              data, 1, TIMEOUT);
      /* ---------------------------------------------------------
      * Step 4 — Handle USB errors
      * --------------------------------------------------------- */
      if (retval < 0) {
        printk(KERN_ERR "ELE784 -> IOCTL_PANTILT_RESET: usb_control_msg failed (%d)\n",retval);
        if (data) {
          kfree(data);
          data = NULL;
        }
        break;
      }
      /* Success return value */
      retval = 0;
      /* ---------------------------------------------------------
      * Step 5 — Free buffer
      * --------------------------------------------------------- */
      if (data) {
        kfree(data);
        data = NULL;
      }
      break;

    case IOCTL_PANTILT_RELATIVE:
    {
      printk(KERN_INFO "ELE784 -> IOCTL_PANTILT_RELATIVE\n");

      /* ---------------------------------------------------------
      * Step 1 — Copy user-space structure (relative pan/tilt)
      * --------------------------------------------------------- */
      struct pantilt_relative rel;
      // read 4 bytes (two int16) from user space
      if (copy_from_user(&rel, (void __user *)arg, sizeof(rel))){
        printk(KERN_ERR "ELE784 -> IOCTL_PANTILT_RELATIVE: copy_from_user(rel) failed\n");
        retval = -EFAULT;
        break;
      }

      /* ---------------------------------------------------------
      * Step 2 — Allocate 4-byte payload buffer
      * --------------------------------------------------------- */
      data = kmalloc(4, GFP_KERNEL);
      if (!data) {
        printk(KERN_ERR "ELE784 -> IOCTL_PANTILT_RESET: kmalloc(4) failed\n");
        retval = -ENOMEM;
        break;
      }

      /* ---------------------------------------------------------
      * Step 3 — Fill payload (little-endian pan/tilt)
      * --------------------------------------------------------- */
      data[0] = rel.pan & 0xFF;
      data[1] = (rel.pan >> 8) & 0xFF;
      data[2] = rel.tilt & 0xFF;
      data[3] = (rel.tilt >> 8) & 0xFF;
      /* ---------------------------------------------------------
      * Step 4 — Send USB SET_CUR request (host → device)
      * --------------------------------------------------------- */      
      retval = usb_control_msg(udev,
                               usb_sndctrlpipe(udev,0),
                               SET_CUR, // bRequest = 1
                               USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE,
                               PANTILT_RELATIVE_CONTROL,
                               PANTILT_INDEX,
                               data,
                               4,
                               TIMEOUT);
      /* ---------------------------------------------------------
      * Step 5 — Handle USB errors
      * --------------------------------------------------------- */
      if (retval < 0) {
          printk(KERN_ERR "ELE784 -> IOCTL_PANTILT_RELATIVE: usb_control_msg failed (%d)\n",
                retval);
          if (data) {
              kfree(data);
              data = NULL;
          }
          break;
      }

      /* ioctl API requires 0 on success */
      retval = 0;
      /* ---------------------------------------------------------
      * Step 6 — Free payload buffer
      * --------------------------------------------------------- */
      if (data) {
        kfree(data);
        data = NULL;
      }
      break;
    }
    
    // Handle IOCTL_STREAMON command
    case IOCTL_STREAMON:

      printk(KERN_INFO "ELE784 -> IOCTL_STREAMON\n");
      {	
        /* ======================================================
         *  PROBE / DEF / SET_CUR / GET_CUR / COMMIT CONTROL
         * ====================================================== */

        // allocation data buffer for 34 bytes (VS_PROBE_CONTROL message length =  VS_PROBE_CONTROL_SIZE)
        data = kmalloc( VS_PROBE_CONTROL_SIZE, GFP_KERNEL);
        if (!data) {
          printk(KERN_ERR "ELE784 -> IOCTL_STREAMON : kmalloc(%d) failed\n", VS_PROBE_CONTROL_SIZE);
          retval = -ENOMEM;
          break;
        }

        // 1 : PROBE_CONTROL(GET_CUR) garbage. Not recommended, used for debug and investigation
        retval = usb_control_msg(
            udev,
            usb_rcvctrlpipe(udev, 0),                         // pipe de contrôle IN
            GET_CUR,                                          // bRequest = 0x81
            USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE,// bmRequestType = 0xA1
            VS_PROBE_CONTROL_VALUE,                           // wValue = 0x0100 (Probe)
            VS_PROBE_CONTROL_WINDEX_LE,                        // wIndex = interface 1, entity 0 (0x0001)
            data,                                             // BUFFER
            VS_PROBE_CONTROL_SIZE,                            // wLength = 34     
            TIMEOUT                                              // timeout (ms)
        );
        if (retval < 0) {
          printk(KERN_ERR "ELE784 -> IOCTL_STREAMON : usb_control_msg(GET_CUR,PROBE) failed, retval=%d\n",retval);
          if(data){
            kfree(data);
            data = NULL;
          }
          break;
        }
        printk(KERN_INFO "ELE784 -> IOCTL_STREAMON : PROBE_CONTROL(GET_CUR) returned %d bytes:\n", retval);
        // print_probe_control_struct(data);
        //SHOUL SHOW WHAT GARBAGE THE DEVICE SENDS

        // 2 : PROBE_CONTROL(GET_DEF) 
        retval = usb_control_msg(
            udev,
            usb_rcvctrlpipe(udev, 0),                         // pipe de contrôle IN
            GET_DEF,                                          // bRequest = 0x81
            USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE,// bmRequestType = 0xA1
            VS_PROBE_CONTROL_VALUE,                           // wValue = 0x0100 (Probe)
            VS_PROBE_CONTROL_WINDEX_LE,                        // wIndex = interface 1, entity 0 (0x0001)
            data,                                             // BUFFER
            VS_PROBE_CONTROL_SIZE,                            // wLength = 34     
            TIMEOUT                                              // timeout (ms)
        );
        if (retval < 0) {
          printk(KERN_ERR "ELE784 -> IOCTL_STREAMON : usb_control_msg(GET_DEF,PROBE) failed, retval=%d\n",retval);
          if(data){
            kfree(data);
            data = NULL;
          }
          break;
        }
        printk(KERN_INFO "ELE784 -> IOCTL_STREAMON : PROBE_CONTROL(GET_DEF) returned %d bytes:\n", retval);
        // print_probe_control_struct(data);



        struct vs_probe_control probe;
        /* Basic hint: host provides frame interval */
        probe.bmHint          = 1;          // keep same
        probe.bFormatIndex    = FORMAT_INDEX_UNCOMPRESSED_YUYV;// FORMAT_UNCOMPRESSED (YUY2)
        // probe.bFrameIndex     = FRAME_INDEX_320x240;          // 320*240 frame
        // probe.bFrameIndex     = FRAME_INDEX_160x120 ;      // <-- 160*120 frame
        probe.bFrameIndex     = FRAME_INDEX_640x480;    // <-- 640x480 frame
        probe.dwFrameInterval = FRAME_INTERVAL_30FPS;     // 30 fps (from descriptor)
        /* Leave these zero for uncompressed */
        probe.wKeyFrameRate   = 0;
        probe.wPFrameRate     = 0;
        probe.wCompQuality    = 0;
        probe.wCompWindowSize = 0;
        probe.wDelay          = 0;
        /* From lsusb for 320x240 UNCOMPRESSED */
        // probe.dwMaxVideoFrameSize  = FRAME_SIZE_320x240; // 320*240*2
        // probe.dwMaxVideoFrameSize      = FRAME_SIZE_160x120 ;   // 160*120*2
        probe.dwMaxVideoFrameSize      = FRAME_SIZE_640x480 ;   // 160*120*2
        // probe.dwMaxPayloadTransferSize = PAYLOAD_SIZE_3060 ; // e.g. 3060
        /* From VC header */
        // probe.dwClockFrequency = CLOCK_FREQUENCY_300MHZ; // 300 MHz
        probe.bmFramingInfo    = 0;
        probe.bPreferedVersion = 0;
        probe.bMinVersion      = 0;
        probe.bMaxVersion      = 0;

        pack_probe_control(&probe, data);
        printk(KERN_INFO "ELE784 -> IOCTL_STREAMON : PROBE_CONTROL(SET_CUR) will send :\n");
        // print_probe_control_struct(data);

        // MODIFY DATA BASED ON THE OUPUT OF THE FIRST GET_CUR. CONFIRM THAT THE DATA TO SEND IS CORRECT AND THEN SEND IT
        // 3 : PROBE_CONTROL (SET_CUR)
        retval = usb_control_msg(
            udev,
            usb_sndctrlpipe(udev, 0),                         
            SET_CUR,                                         
            USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE,
            VS_PROBE_CONTROL_VALUE,                                
            VS_PROBE_CONTROL_WINDEX_LE,                                           
            data,                                             
            VS_PROBE_CONTROL_SIZE,                                       
            TIMEOUT                                              
        );
        if (retval < 0) {
          printk(KERN_ERR "ELE784 -> IOCTL_STREAMON : usb_control_msg(SET_CUR/PROBE) failed, retval=%d\n",retval);
          if(data){
            kfree(data);
            data = NULL;
          }
          break;
        }
        printk(KERN_INFO "ELE784 -> IOCTL_STREAMON : PROBE_CONTROL(SET_CUR) returned %d bytes:\n", retval);


        // 4.PROBE_CONTROL (GET_CUR)
        retval = usb_control_msg(
            udev,
            usb_rcvctrlpipe(udev, 0),                         // pipe de contrôle IN
            GET_CUR,                                          // bRequest = 0x81
            USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE,// bmRequestType = 0xA1
            VS_PROBE_CONTROL_VALUE,                           // wValue = 0x0100 (Probe)
            VS_PROBE_CONTROL_WINDEX_LE,                        // wIndex = interface 1, entity 0 
            data,                                             // BUFFER
            VS_PROBE_CONTROL_SIZE,                            // wLength = 34     
            TIMEOUT                                           // timeout (ms)
        );
        if (retval < 0) {
          printk(KERN_ERR "ELE784 -> IOCTL_STREAMON : usb_control_msg(GET_CUR,PROBE) failed, retval=%d\n",retval);
          if(data){
            kfree(data);
            data = NULL;
          }
          break;
        }
        printk(KERN_INFO "ELE784 -> IOCTL_STREAMON : PROBE_CONTROL(GET_CUR) returned %d bytes:\n", retval);
        // print_probe_control_struct(data);

        /* 4) SET_CUR(COMMIT) – commit the settings */
        // this dont have the rigth format 
        retval = usb_control_msg(
            udev,
            usb_sndctrlpipe(udev, 0),
            SET_CUR,
            USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE,
            VS_COMMIT_CONTROL_VALUE,
            VS_COMMIT_CONTROL_WINDEX_LE,
            data,
            VS_PROBE_CONTROL_SIZE,
            TIMEOUT);
        if (retval < 0) {
            printk(KERN_ERR "STREAMON: SET_CUR(COMMIT) failed (%d)\n", retval);
            kfree(data);
            data = NULL;
            break;
        }

        /* ======= END PROBE/COMMIT ======= */


        uint32_t bandwidth, psize, size, npackets, urb_size;
        struct usb_host_endpoint *ep = NULL;
        struct usb_host_interface *alts;
        int	   best_altset;

        printk(KERN_INFO "ELE784 -> IOCTL_STREAMON: VS interface #1 has %u altsettings\n",interface->num_altsetting);
        
        //not sure about the bandwidth and size calculation, need to check with teacher's macros
        // avec les infos du get_cur

        // À partir des données de configurations obtenues, détermine la Bande Passante et la taille des transferts :	  
        bandwidth = (((uint32_t) data[25]) << 24) | (((uint32_t) data[24]) << 16) | (((uint32_t) data[23]) << 8) | (((uint32_t) data[22]) << 0);
        size      = (((uint32_t) data[21]) << 24) | (((uint32_t) data[20]) << 16) | (((uint32_t) data[19]) << 8) | (((uint32_t) data[18]) << 0);
        printk(KERN_INFO "ELE784 -> IOCTL_STREAMON: bandwidth = %u  size = %u\n",bandwidth, size);
        // Selon la Bande Passante et la taille des Paquets, trouve la meilleure "Interface Alternative" a utiliser (dépend de la résolution Video choisie)
        // Note :	Chacune de ces "Interfaces Alternatives" n'a qu'un seul Endpoint...donc on conserve l'info sur ce Endpoint.
        for (best_altset = 0; best_altset < interface->num_altsetting; best_altset++) {
          alts = &(interface->altsetting[best_altset]);
          if (alts->desc.bNumEndpoints < 1)
            continue;
          ep = &(alts->endpoint[0]);


          /* Skip anything that is NOT isochronous */
          if (!usb_endpoint_xfer_isoc(&ep->desc)) {
              printk(KERN_INFO "ELE784 -> alt=%d: not isochronous, skipping\n",best_altset);
              ep = NULL;
              continue;
          }
          psize = (ep->desc.wMaxPacketSize & 0x07ff) * (((ep->desc.wMaxPacketSize >> 11) & 0x0003) + 1);
          if (psize >= bandwidth)
            break;
        }
        if (ep == NULL) {
          printk(KERN_WARNING "ELE784 -> IOCTL_STREAMON : No Endpoint found error");
          if(data){
            kfree(data);
            data = NULL;
          }
          retval = -ENOMEM;
          break;
        }
            
        // Avec l'interface choisie, on détermine le nombre de Paquets que chaque Urb aura à transporter.
        npackets = ((size % psize) > 0) ? (size/psize + 1) : (size/psize);

        //need to clamp npackets to MAX_PACKETS
        if (npackets > MAX_PACKETS) {
          npackets = MAX_PACKETS;
        }
        urb_size = psize*npackets;
        printk(KERN_INFO "ELE784 -> IOCTL_STREAMON : bandwidth = %u psize = %u npackets = %u urb_size = %u best_altset = %u\n", bandwidth, psize, npackets, urb_size, best_altset);
        // Et on alloue dynamiquement (obligatoire) le tampon où seront placées les données récoltées par les Urbs.   		
        driver->frame_buf.Data = kmalloc(size, GFP_KERNEL);
        if (driver->frame_buf.Data == NULL) {
          // printk(KERN_WARNING "ELE784 -> IOCTL_STREAMON : No memory for URB buffer[0]");
          printk(KERN_WARNING "ELE784 -> IOCTL_STREAMON : No memory for frame buffer (%u bytes)\n", size);
          if(data){
            kfree(data);
            data =NULL;
          }
          retval = -ENOMEM;
          break;
        }
        driver->frame_buf.MaxLength = size;
        driver->frame_buf.BytesUsed = 0; 
        driver->frame_buf.LastFID = -1;
        driver->frame_buf.Status = 0;  // <-- IMPORTANT: Initialize to 0
        driver->frame_buf.LastFID = -1; // <-- Initialize ONCE during STREAMON


        // On a besoin d'un mécanisme de synchro pour la détection du début d'un "Frame" (une image) et la détection de la fin de chaque Urb.
        init_completion(&(driver->frame_buf.new_frame_start));
        init_completion(&(driver->frame_buf.urb_completion));
            

        driver->frame_buf.Status |= BUF_STREAM_READ;  // <-- ADD THIS LINE!
    
        // Ici, on rend "courante" l'interface alternative choisie comme étant la meilleure.
        retval = usb_set_interface(udev, 1, best_altset); //Important pour mettre la camera dans le bon mode
        if (retval < 0) {
            if (data){
              kfree(data);
              data=NULL;
            }
            if (driver->frame_buf.Data){ 
              kfree(driver->frame_buf.Data);
              driver->frame_buf.Data = NULL;
            }
            break;
        }else{
          printk(KERN_INFO "ELE784 -> IOCTL_STREAMON : usb_set_interface successful retval = %d\n",retval);
        }
        // Finalement, on créé les Urbs Isochronous (un total de URB_COUNT Urbs).
        for (i = 0; i < URB_COUNT; i++) {
          driver->isoc_in_urb[i] = usb_alloc_urb(npackets, GFP_KERNEL);
          if (driver->isoc_in_urb[i] == NULL) {
            printk(KERN_WARNING "ELE784 -> IOCTL_STREAMON : URB allocation error");
            if(data){
              kfree(data);
              data =NULL;
            }
            if (driver->frame_buf.Data) {
              kfree(driver->frame_buf.Data);
              driver->frame_buf.Data = NULL;
            }
            retval = -ENOMEM;
            break;
          }

          driver->isoc_in_urb[i]->transfer_buffer = usb_alloc_coherent(udev, urb_size, GFP_KERNEL, &(driver->isoc_in_urb[i]->transfer_dma));
          if (driver->isoc_in_urb[i]->transfer_buffer == NULL) {
            printk(KERN_WARNING "ELE784 -> IOCTL_STREAMON : Transfert buffer allocation error");
            usb_free_urb(driver->isoc_in_urb[i]);
            if(data){
              kfree(data);
              data =NULL;
            }
            if (driver->frame_buf.Data) {
              kfree(driver->frame_buf.Data);
              driver->frame_buf.Data = NULL;
            }
            retval = -ENOMEM;
            break;
          }

          /*******************************************************************************
          Ici, il s'agit d'initialiser l'Urb Isochronous (voir acétate 16 du cours # 5)
          Suggestion :	Attacher la structure (driver->frame_buf) au champ "context" de la structure du Urb.
          ******* ************************************************************************/
          struct urb *urb = driver->isoc_in_urb[i];
          /* Pointeur vers le device */
          urb->dev = udev;
          /* Contexte transmis au callback */
          urb->context = &(driver->frame_buf);
          /* Endpoint Isochronous IN */
          urb->pipe = usb_rcvisocpipe(udev, ep->desc.bEndpointAddress);
          /* Flags recommandés */
          urb->transfer_flags = URB_ISO_ASAP | URB_NO_TRANSFER_DMA_MAP;
          /* Intervalle d'interrogation (polling) */
          urb->interval = ep->desc.bInterval;
          /* Callback qui sera appelé quand l’URB est complété */
          urb->complete = complete_callback;
          /* Nombre de paquets dans l’URB */
          urb->number_of_packets = npackets;
          /* Taille totale du buffer */
          urb->transfer_buffer_length = urb_size;
          /* Configurer les paquets ISO individuellement */
          for (j = 0; j < npackets; j++) {
              urb->iso_frame_desc[j].offset = j * psize;
              urb->iso_frame_desc[j].length = psize;
          }
        }
        
        // Et on lance tous les Urbs créés. 
        /*******************************************************************************
        Ici, il s'agit de soumettre tous les Urbs créés ci-dessus.
        *******************************************************************************/
        /* ====== ÉTAPE 3 : Soumission des URBs ====== */
        for (i = 0; i < URB_COUNT; i++) {
          retval = usb_submit_urb(driver->isoc_in_urb[i], GFP_KERNEL);
          if (retval < 0) {
            printk(KERN_ERR "ELE784 -> IOCTL_STREAMON: usb_submit_urb[%d] failed (%d)\n",i, retval);
            /* First, free the current (failed) URB i */
            usb_free_coherent(udev,
                urb_size,
                driver->isoc_in_urb[i]->transfer_buffer,
                driver->isoc_in_urb[i]->transfer_dma);
            usb_free_urb(driver->isoc_in_urb[i]);
            driver->isoc_in_urb[i] = NULL;
            /* rollback: kill + free previously submitted URBs   0..i-1*/
            while (--i >= 0) {
              usb_kill_urb(driver->isoc_in_urb[i]);
              usb_free_coherent(udev,
                  urb_size,
                  driver->isoc_in_urb[i]->transfer_buffer,
                  driver->isoc_in_urb[i]->transfer_dma);
              usb_free_urb(driver->isoc_in_urb[i]);
              driver->isoc_in_urb[i] = NULL;
            }
            /* free frame buffer */
            if (driver->frame_buf.Data) {
                kfree(driver->frame_buf.Data);
                driver->frame_buf.Data = NULL;
            }
            /* free probe data */
            if (data) {
                kfree(data);
                data = NULL;
            }
            break; /* leave STREAMON case */
          }
        } 

        if (retval >= 0) {
          printk(KERN_INFO "ELE784 -> IOCTL_STREAMON: streaming started (%d URBs submitted)\n",URB_COUNT);
        }
      
      } 
      if (data) {
        kfree(data);
        data = NULL;
      }
      break;

    // Handle IOCTL_STREAMOFF command
  case IOCTL_STREAMOFF:
      printk(KERN_INFO "ELE784 -> IOCTL_STREAMOFF\n");

      /* 1) Kill all URBs */
      for (i = 0; i < URB_COUNT; i++) {
        if (driver->isoc_in_urb[i]) {
          usb_kill_urb(driver->isoc_in_urb[i]);
        }
      }

      /* 2) Free URB resources */
      for (i = 0; i < URB_COUNT; i++) {
        if (driver->isoc_in_urb[i]) {
          usb_free_coherent(udev,
                // urb_size,
                // driver->frame_buf.MaxLength,
                driver->isoc_in_urb[i]->transfer_buffer_length,
                driver->isoc_in_urb[i]->transfer_buffer,
                driver->isoc_in_urb[i]->transfer_dma);

          usb_free_urb(driver->isoc_in_urb[i]);
          driver->isoc_in_urb[i] = NULL;
        }
      }

      /* 3) Free frame buffer */
      if (driver->frame_buf.Data) {
        kfree(driver->frame_buf.Data);
        driver->frame_buf.Data = NULL;
      }

      /* 4) Set altsetting 0 (stop streaming) */
      usb_set_interface(udev, 1, 0);

      /* 5) Reset completions */
      reinit_completion(&driver->frame_buf.new_frame_start);
      reinit_completion(&driver->frame_buf.urb_completion);

      retval = 0;
      break;

    default:
      printk(KERN_WARNING "ELE784 -> IOCTL Error\n");
      retval = -EINVAL;
      break;
  }
  return retval;
}


//this is the committed version of ele784_read()
ssize_t ele784_read(struct file *file,char __user *buffer,size_t count,loff_t *f_pos)
{
    struct orbit_driver *dev = file->private_data;
    struct driver_buffer *fb;
    size_t bytes_to_copy;

    if (!dev)
        return -ENODEV;

    fb = &dev->frame_buf;

    // =====================================================
    // CRITICAL FIX: Wait for FID toggle BEFORE resetting
    // =====================================================
    
    // Wait for the START of a new frame (FID toggle)
    if (wait_for_completion_interruptible(&fb->new_frame_start)) {
        return -ERESTARTSYS;
    }
    // NOW reset for this frame (callback has already started filling buffer)
    // But DON'T reset LastFID - let callback track it across frames
    reinit_completion(&fb->new_frame_start);
    reinit_completion(&fb->urb_completion);


    // =====================================================
    // Wait until callback signals EOF of this frame
    // =====================================================
    while (!(fb->Status & BUF_STREAM_EOF)) {

      if (wait_for_completion_interruptible(&fb->urb_completion)) {
        fb->Status &= ~(BUF_STREAM_READ | BUF_STREAM_EOF | BUF_STREAM_FRAME_READ);
        return -ERESTARTSYS;
      }

      reinit_completion(&fb->urb_completion);
    }

    // =====================================================
    // COPY FRAME TO USER BUFFER
    // =====================================================
    bytes_to_copy = min((size_t)fb->BytesUsed, count);

    if (copy_to_user(buffer, fb->Data, bytes_to_copy)) {
      fb->Status &= ~(BUF_STREAM_READ | BUF_STREAM_EOF | BUF_STREAM_FRAME_READ);
      fb->BytesUsed = 0;
      return -EFAULT;
    }
    // printk(KERN_INFO "ELE784 -> read() returning %zu bytes\n", bytes_to_copy);

    // =====================================================
    // CLEAN UP FOR NEXT READ()
    // =====================================================
    // fb->Status &= ~(BUF_STREAM_READ | BUF_STREAM_EOF | BUF_STREAM_FRAME_READ);
    // fb->Status &= ~(BUF_STREAM_EOF | BUF_STREAM_FRAME_READ);
    fb->Status &= ~BUF_STREAM_EOF;

    return bytes_to_copy;
}