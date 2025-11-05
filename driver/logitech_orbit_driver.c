#include "logitech_orbit_driver.h"


// Event when the device is opened
int ele784_open(struct inode *inode, struct file *file) {
  struct usb_interface *interface;
  int subminor;

  printk(KERN_WARNING "ELE784 -> Open\n");

  subminor = iminor(inode);
  interface = usb_find_interface(&udriver, subminor);
  if (!interface) {
    printk(KERN_WARNING "ELE784 -> Open: Ne peux ouvrir le peripherique\n");
    return -ENODEV;
  }

  file->private_data = usb_get_intfdata(interface);

  return 0;
}

// Probing USB device
int ele784_probe(struct usb_interface *interface, const struct usb_device_id *id) {
  struct orbit_driver *dev;
  struct usb_host_interface *iface_desc;
  int i;
  int retval = -ENOMEM;

  printk(KERN_INFO "ELE784 -> Probe\n");

  dev = kzalloc(sizeof(*dev), GFP_KERNEL);
  if (!dev) {
    dev_err(&interface->dev, "Out of memory\n");
    return -ENOMEM;
  }

  dev->device = usb_get_dev(interface_to_usbdev(interface));
  dev->interface = interface;

  for (i = 0; i < URB_COUNT; ++i) {
    dev->isoc_in_urb[i] = NULL;
  }

  iface_desc = interface->cur_altsetting;

/*******************************************************************************
	Dans cette partie, il faut détecter les interfaces désirées et s'y attacher.
	Les interfaces que nous voulons sont :
								Video Streaming
								Video Control
*******************************************************************************/

  return retval;
}


void ele784_disconnect (struct usb_interface *intf) {
/*******************************************************************************
Ici, il faut s'assurer d'éliminer tous Urb en cours et se détacher de l'interface
*******************************************************************************/
}


long ele784_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
  struct orbit_driver  *driver = (struct orbit_driver *) file->private_data;
  struct usb_interface *interface = driver->interface;
  struct usb_device    *udev = interface_to_usbdev(interface);

  struct usb_request user_request;
  uint8_t  request, data_size;
  uint16_t value, index, timeout;
  uint8_t  *data;

  int i,j;
  long retval=0;

  switch(cmd) {

  case IOCTL_GET:
/*******************************************************************************
Ici, il faut transmettre la requête de l'usager et retourner à l'usager la réponse du périphérique.
	(voir la commande IOCTL_SET ci-dessous comme exemple de transmission d'une requête)
*******************************************************************************/
   break;

  case IOCTL_SET:
    printk(KERN_INFO "ELE784 -> IOCTL_SET\n");

    copy_from_user(&user_request, (struct usb_request *)arg, sizeof(struct usb_request));

    data_size = user_request.data_size;
    request   = user_request.request;
    value     = (user_request.value) << 8;
    index     = (user_request.index) << 8 | interface->cur_altsetting->desc.bInterfaceNumber;
    timeout   = user_request.timeout;
    data      = NULL;

    if (data_size > 0) {
  		data = kmalloc(data_size, GFP_KERNEL);
    	copy_from_user(data, ((uint8_t __user *) user_request.data), data_size*sizeof(uint8_t));
    }
    
    retval = usb_control_msg(udev,
                             usb_sndctrlpipe(udev, 0x00),
                             request,
                             USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE,
                             value, index, data, data_size, timeout);
    if (data) {
    	kfree(data);
    	data = NULL;
    }
    
    break;

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
	{	uint32_t bandwidth, psize, size, npackets, urb_size;
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
			ep   = &(alts->endpoint[0]);
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
*******************************************************************************/

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

  case IOCTL_STREAMOFF:
    printk(KERN_INFO "ELE784 -> IOCTL_STREAMOFF\n");
/*******************************************************************************
Ici, il faut "tuer" et éliminer tous les Urbs qui sont actifs et arrêter le Streaming de la caméra.
Pour arrêter le Streaming de la caméra, il suffit de rendre "courant" l'interface alternative 0.
*******************************************************************************/
    break;

  case IOCTL_PANTILT_RELATIVE:
/*******************************************************************************
Ici, il faut transmettre le tableau [Pan, Tilt] reçu de l'usager 
*******************************************************************************/
    break;

  case IOCTL_PANTILT_RESET:
    // Commande propriétaire Logitech pour recentrer la caméra
    // Allouer dynamiquement le buffer (obligation USB)
    uint8_t *buffer = kmalloc(1, GFP_KERNEL);
    if (!buffer)
        return -ENOMEM;

    // Payload: commande reset propriétaire
    #define PANTILT_RESET_CMD     0x03
    #define PANTILT_RESET_VALUE   (0x02 << 8)  // Sélecteur propriétaire
    #define PANTILT_RESET_INDEX   0x0900       // Interface vidéo + classe
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
    break;

  default:
    printk(KERN_WARNING "ELE784 -> IOCTL Error\n");
    retval = -EINVAL;
    break;
  }

  return retval;
}

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
