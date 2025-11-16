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

  printk(KERN_INFO "ELE784 -> Probe interface number: %d\n", iface_desc->desc.bInterfaceNumber);
  printk(KERN_INFO "ELE784 -> Interface class: 0x%02x, subclass: 0x%02x, protocol: 0x%02x\n",
         iface_desc->desc.bInterfaceClass,
         iface_desc->desc.bInterfaceSubClass,
         iface_desc->desc.bInterfaceProtocol);

  // Print all alternate settings and endpoint info for this interface
  for (i = 0; i < interface->num_altsetting; i++) {
    struct usb_host_interface *alts = &interface->altsetting[i];
    printk(KERN_INFO "ELE784 -> AltSetting %d: bNumEndpoints=%d\n",alts->desc.bAlternateSetting, alts->desc.bNumEndpoints);
    for (j = 0; j < alts->desc.bNumEndpoints; j++) {
      struct usb_endpoint_descriptor *epd = &alts->endpoint[j].desc;
      printk(KERN_INFO "ELE784 ->  Endpoint %d: addr=0x%02x, attr=0x%02x, maxpacket=%d\n",j, epd->bEndpointAddress, epd->bmAttributes, epd->wMaxPacketSize);
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
  uint8_t  *data; // data buffer pointer

  int i,j; // loop counters
  long retval=0; // return value

  // Handle different IOCTL commands
  switch(cmd) {

    // Handle IOCTL_GET command 
    case IOCTL_GET:
      printk(KERN_INFO "ELE784 -> IOCTL_GET\n");
      // 1. Copy request from user space
      // - 'arg' is a pointer to a usb_request structure in user space.
      // - copy_from_user() safely copies it into kernel memory.
      // - This avoids directly dereferencing user-space pointers, which would crash the kernel.
      if (copy_from_user(&user_request, (struct usb_request __user *)arg, sizeof(struct usb_request))) {
        // Error handling if copy fails
        printk(KERN_ERR "ELE784 -> IOCTL_GET : copy_from_user (request) failed\n");
        retval = -EFAULT;
        break;
      }
      /* Allocate kernel buffer for incoming data */
      data_size = user_request.data_size;
      data = NULL;                   // Pointer for buffer to receive data
      // 3. Allocate buffer if data_size > 0
      if (data_size > 0) {
        // allocate memory in kernel space
        data = kmalloc(data_size, GFP_KERNEL);
        // check for allocation failure
        if (!data) {
          // log error and set return value
          printk(KERN_ERR "ELE784 -> IOCTL_GET : kmalloc failed\n");
          retval = -ENOMEM;
          break;
        }
      }

      /* Perform the USB GET (device → host) */
      retval = usb_control_msg(
          udev,
          usb_rcvctrlpipe(udev, 0),
          user_request.request,   // GET_CUR, GET_MIN, GET_MAX, ...
          USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE,
          user_request.value,     // full 16-bit wValue already provided by user
          user_request.index,     // full 16-bit wIndex (interface) already provided
          data,
          data_size,
          user_request.timeout
      );

      // 5) Copy the received data back to user space
      // - Only copy if usb_control_msg succeeded and there was data to receive
      if (retval >= 0 && data_size > 0) {
        if (copy_to_user(user_request.data, data, data_size)) {
          printk(KERN_ERR "ELE784 -> IOCTL_GET : copy_to_user failed\n");
          retval = -EFAULT; // set error if copying fails
        }
      }    
      // 6) Free the kernel buffer
      // - Dynamically allocated memory must always be freed to avoid memory leaks
      kfree(data);
      data = NULL;
      break; //  Required to exit the switch

    case IOCTL_SET:
        printk(KERN_INFO "ELE784 -> IOCTL_SET\n");

        /* 1. Copy the request struct from user space */
        if (copy_from_user(&user_request,
                          (struct usb_request __user *)arg,
                          sizeof(struct usb_request))) {
            printk(KERN_ERR "ELE784 -> IOCTL_SET: copy_from_user failed\n");
            retval = -EFAULT;
            break;
        }

        /* 2. Allocate payload buffer if needed */
        data_size = user_request.data_size;
        data = NULL;

        if (data_size > 0) {
            data = kmalloc(data_size, GFP_KERNEL);
            if (!data) {
                retval = -ENOMEM;
                break;
            }

            /* Copy payload (host → device) */
            if (copy_from_user(data,
                              (uint8_t __user *)user_request.data,
                              data_size)) {
                printk(KERN_ERR "ELE784 -> IOCTL_SET: copy_from_user(data) failed\n");
                kfree(data);
                retval = -EFAULT;
                break;
            }
        }

        /* 3. Perform the USB SET (OUT direction — host → device) */
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
        /* 4. Free payload buffer */
        if (data) {
            kfree(data);
            data = NULL;
        }

        break;

  
    case IOCTL_PANTILT_RESET:
      printk(KERN_INFO "ELE784 -> IOCTL_PANTILT_RESET\n");
      // Commande propriétaire Logitech pour recentrer la caméra
      // Allouer dynamiquement le buffer (obligation USB)
      uint8_t *buffer = kmalloc(1, GFP_KERNEL);
      if (!buffer)
        return -ENOMEM;
      // Payload: commande reset propriétaire
      #define PANTILT_RESET_CMD     0x03
      #define PANTILT_RESET_VALUE   (0x02 << 8)  // Sélecteur propriétaire
      #define PANTILT_RESET_INDEX   0x0B00       // Interface vidéo + classe
      #define PANTILT_RESET_TIMEOUT 400          // Timeout en ms
      buffer[0] = PANTILT_RESET_CMD;
      retval = usb_control_msg(udev,
                              usb_sndctrlpipe(udev, 0),
                              SET_CUR,
                              USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE,
                              PANTILT_RESET_VALUE,
                              PANTILT_RESET_INDEX,
                              buffer, 1, PANTILT_RESET_TIMEOUT);
      kfree(buffer);
      buffer = NULL;
      break;

    case IOCTL_PANTILT_RELATIVE:
    {
      printk(KERN_INFO "ELE784 -> IOCTL_PANTILT_RELATIVE\n");
      struct pantilt_relative rel;

      // read 4 bytes (two int16) from user space
      if (copy_from_user(&rel, (void __user *)arg, sizeof(rel)))
        return -EFAULT;

      // allocate 4 bytes for USB payload
      data = kmalloc(4, GFP_KERNEL);
      if (!data) 
        return -ENOMEM;

      // fill as little-endian
      data[0] = rel.pan & 0xFF;
      data[1] = (rel.pan >> 8) & 0xFF;
      data[2] = rel.tilt & 0xFF;
      data[3] = (rel.tilt >> 8) & 0xFF;

      #define PANTILT_RELATIVE_CONTROL (0x01<<8)
      #define PANTILT_RELATIVE_INDEX 0x0B00
      #define PANTILT_RELATIVE_TIMEOUT 400

      retval = usb_control_msg(udev,
                               usb_sndctrlpipe(udev,0),
                               SET_CUR, // bRequest = 1
                               USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE,
                               PANTILT_RELATIVE_CONTROL,
                               PANTILT_RELATIVE_INDEX,
                               data,
                               4,
                               PANTILT_RELATIVE_TIMEOUT);
                                                       
      kfree(data);
      data = NULL;
      break;
    }
    
    // Handle IOCTL_STREAMON command
    case IOCTL_STREAMON:
      printk(KERN_INFO "ELE784 -> IOCTL_STREAMON\n");
      /*******************************************************************************
      Ici, il faut préparer et transmettre une requête similaire à un IOCTL_GET avec les caractéristiques suivantes :
          data_size = 26
          request   = GET_CUR
          value     = VS_PROBE_CONTROL
          index     = 0x0000
          timeout   = 5000
      Les données récoltées grâce à cette requête seront ensuite utilisées pour configurer les requêtes Urb (voir ci-dessous).
      *******************************************************************************/
      {	
        uint32_t bandwidth, psize, size, npackets, urb_size;
        struct usb_host_endpoint *ep = NULL;
        struct usb_host_interface *alts;
        int	   best_altset;
        // À partir des données de configurations obtenues, détermine la Bande Passante et la taille des transferts :	  
        bandwidth = (((uint32_t) data[25]) << 24) | (((uint32_t) data[24]) << 16) | (((uint32_t) data[23]) << 8) | (((uint32_t) data[22]) << 0);
        size      = (((uint32_t) data[21]) << 24) | (((uint32_t) data[20]) << 16) | (((uint32_t) data[19]) << 8) | (((uint32_t) data[18]) << 0);

        // Selon la Bande Passante et la taille des Paquets, trouve la meilleure "Interface Alternative" a utiliser (dépend de la résolution Video choisie)
        // Note :	Chacune de ces "Interfaces Alternatives" n'a qu'un seul Endpoint...donc on conserve l'info sur ce Endpoint.
        for (best_altset = 0; best_altset < interface->num_altsetting; best_altset++) {
          alts = &(interface->altsetting[best_altset]);
          if (alts->desc.bNumEndpoints < 1)
            continue;
          ep = &(alts->endpoint[0]);
          psize = (ep->desc.wMaxPacketSize & 0x07ff) * (((ep->desc.wMaxPacketSize >> 11) & 0x0003) + 1);
          if (psize >= bandwidth)
            break;
        }
        if (ep == NULL) {
          printk(KERN_WARNING "ELE784 -> IOCTL_STREAMON : No Endpoint found error");
          return -ENOMEM;
        }
            
        // Avec l'interface choisie, on détermine le nombre de Paquets que chaque Urb aura à transporter.
        npackets = ((size % psize) > 0) ? (size/psize + 1) : (size/psize);
        npackets = (npackets > MAX_PACKETS) ? MAX_PACKETS : npackets;
        urb_size = psize*npackets;

        // Et on alloue dynamiquement (obligatoire) le tampon où seront placées les données récoltées par les Urbs.   		
        driver->frame_buf.Data = kmalloc(urb_size, GFP_KERNEL);
        if (driver->frame_buf.Data == NULL) {
          printk(KERN_WARNING "ELE784 -> IOCTL_STREAMON : No memory for URB buffer[0]");
          return -ENOMEM;
        }
        driver->frame_buf.MaxLength = urb_size;
        driver->frame_buf.BytesUsed = 0; 
        driver->frame_buf.LastFID = -1;

        // On a besoin d'un mécanisme de synchro pour la détection du début d'un "Frame" (une image) et la détection de la fin de chaque Urb.
        init_completion(&(driver->frame_buf.new_frame_start));
        init_completion(&(driver->frame_buf.urb_completion));
            
        printk(KERN_INFO "ELE784 -> IOCTL_STREAMON : bandwidth = %u psize = %u npackets = %u urb_size = %u best_altset = %u\n", bandwidth, psize, npackets, urb_size, best_altset);
        // Ici, on rend "courante" l'interface alternative choisie comme étant la meilleure.
        retval = usb_set_interface(udev, 1, best_altset); //Important pour mettre la camera dans le bon mode

        // Finalement, on créé les Urbs Isochronous (un total de URB_COUNT Urbs).
        for (i = 0; i < URB_COUNT; i++) {
          driver->isoc_in_urb[i] = usb_alloc_urb(npackets, GFP_KERNEL);
          if (driver->isoc_in_urb[i] == NULL) {
            printk(KERN_WARNING "ELE784 -> IOCTL_STREAMON : URB allocation error");
            return -ENOMEM;
          }

          driver->isoc_in_urb[i]->transfer_buffer = usb_alloc_coherent(udev, urb_size, GFP_KERNEL, &(driver->isoc_in_urb[i]->transfer_dma));
          if (driver->isoc_in_urb[i]->transfer_buffer == NULL) {
            printk(KERN_WARNING "ELE784 -> IOCTL_STREAMON : Transfert buffer allocation error");
            usb_free_urb(driver->isoc_in_urb[i]);
            return -ENOMEM;
          }

        /*******************************************************************************
        Ici, il s'agit d'initialiser l'Urb Isochronous (voir acétate 16 du cours # 5)
        Suggestion :	Attacher la structure (driver->frame_buf) au champ "context" de la structure du Urb.
        ******* ************************************************************************/
        }
        // Et on lance tous les Urbs créés. 
        /*******************************************************************************
        Ici, il s'agit de soumettre tous les Urbs créés ci-dessus.
        *******************************************************************************/
      } 
      if (data) {
        kfree(data);
        data = NULL;
      }
      break;

    // Handle IOCTL_STREAMOFF command
    case IOCTL_STREAMOFF:
      printk(KERN_INFO "ELE784 -> IOCTL_STREAMOFF\n");
      /*******************************************************************************
      Ici, il faut "tuer" et éliminer tous les Urbs qui sont actifs et arrêter le Streaming de la caméra.
      Pour arrêter le Streaming de la caméra, il suffit de rendre "courant" l'interface alternative 0.
      *******************************************************************************/
      break;

    default:
      printk(KERN_WARNING "ELE784 -> IOCTL Error\n");
      retval = -EINVAL;
      break;
  }
  return retval;
}

// Read function for video streaming device
ssize_t ele784_read(struct file *file, char __user *buffer, size_t count, loff_t *f_pos) {
/*******************************************************************************
	Ici, il faut :
	
		1 -	Lever le drapeau de "Status" BUF_STREAM_READ dans le "frame_buf"
			pour mettre le Video Streaming en mode lecture et attendre le début
			d'un nouveau Frame.
			
		2 -	Répéter ce qui suit jusqu'à la détection de la fin du Frame :
					( Drapeau BUF_STREAM_EOF dans Status )

				a -	Attend la fin d'un Urb.

				b -	Récupère les données produites par ce Urb.

				c -	Transmet les données récupérées à l'usager.
				
		3 -	Baisse tous les drapeaux dans Status et retourne le nombre total
			de bytes qui ont été transmit à l'usager.
*******************************************************************************/
  return 0;
}
