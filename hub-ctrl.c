// sudo apt-get install libusb-dev
// gcc -o hub-ctrl hub-ctrl.c -lusb -std=c99

#include <usb.h>
#include <stdio.h>
#include <string.h>

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_RESET   "\x1b[0m"

#define USB_RT_HUB              (USB_TYPE_CLASS | USB_RECIP_DEVICE)
#define USB_RT_PORT             (USB_TYPE_CLASS | USB_RECIP_OTHER)
#define USB_PORT_FEAT_POWER     8
#define USB_DIR_IN              0x80        /* to host */

#define HUB_CHAR_LPSM           0x0003
#define HUB_CHAR_PORTIND        0x0080
#define CTRL_TIMEOUT 1000
#define USB_STATUS_SIZE 4

#define MAX_HUBS 128

struct hub_metadata {
  unsigned char bDescLength;
  unsigned char bDescriptorType;
  unsigned char bNbrPorts;
  unsigned char wHubCharacteristics[2];
  unsigned char bPwrOn2PwrGood;
  unsigned char bHubContrCurrent;
  unsigned char data[0];
};

struct hub_info {
  int busnum;
  int devnum;
  struct usb_device *dev;
  int port_count;
};

static struct hub_info hubs[MAX_HUBS];
static int hub_count = 0;

static void exit_with_usage (const char *progname) {
  printf ("Usage: %s [-H <Hub> | -B <Bus> -D <Dev>] -P <Port> -p <0|1>\n", progname);
  exit (1);
}

static void list_ports (usb_dev_handle *uh, int port_count) {
  for (int i = 1; i <= port_count; i++) {
    char buf[USB_STATUS_SIZE];
    if (usb_control_msg (uh, USB_ENDPOINT_IN | USB_TYPE_CLASS | USB_RECIP_OTHER, USB_REQ_GET_STATUS, 0, i, buf, USB_STATUS_SIZE, CTRL_TIMEOUT) < 0) {
      printf (ANSI_COLOR_RED "> Cannot read port %d status\n" ANSI_COLOR_RESET, i);
      break;
    }

    if (i == port_count) { // last item in the list
      printf(" └");
    }
    else {
      printf(" ├");
    }
    printf("─ Port %2d: %s%s%s%s%s%s%s%s%s%s\n", i,
              (buf[1] & 0x01) ? " power" : "",
              (buf[1] & 0x02) ? " lowspeed" : "",
              (buf[1] & 0x04) ? " highspeed" : "",
              (buf[1] & 0x08) ? " test" : "",
              (buf[1] & 0x10) ? " indicator" : "",
              (buf[0] & 0x01) ? " connect" : "",
              (buf[0] & 0x02) ? " enable" : "",
              (buf[0] & 0x04) ? " suspend" : "",
              (buf[0] & 0x08) ? " oc" : "",
              (buf[0] & 0x10) ? " RESET" : "");
  }
  printf(ANSI_COLOR_RESET);
}

static void list_hubs (int port) {
  struct usb_bus *bus;

  bus = usb_get_busses();
  if (bus == NULL) {
    printf (ANSI_COLOR_RED "> Failed to access USB bus." ANSI_COLOR_RESET);
    exit(1);
  }

  for (bus; bus; bus = bus->next) {
    struct usb_device *dev;
    for (dev = bus->devices; dev; dev = dev->next) {
      if (dev->descriptor.bDeviceClass != USB_CLASS_HUB) {
        continue;
      }
      usb_dev_handle *uh;
      uh = usb_open (dev);

      if (uh != NULL) {
          char buf[1024];
          if (port == 0) {
            int len;
            struct hub_metadata *hub_data = (struct hub_metadata *)buf;
            if ((len = usb_control_msg (uh, USB_DIR_IN | USB_RT_HUB, USB_REQ_GET_DESCRIPTOR, USB_DT_HUB << 8, 0, buf, sizeof (buf), CTRL_TIMEOUT)) > sizeof (struct hub_metadata)) {
              printf ("Hub %d (Bus %d, Dev %d) ", hub_count, atoi(bus->dirname), dev->devnum);
              switch ((hub_data->wHubCharacteristics[0] & HUB_CHAR_LPSM)) {
                case 0:
                  printf (ANSI_COLOR_YELLOW "- ganged power switching\n");
                  break;
                case 1:
                  printf (ANSI_COLOR_GREEN "- individual power switching\n");
                  break;
                case 2:
                case 3:
                  printf (ANSI_COLOR_RED "- no power switching\n");
                  break;
              }
            }
            else {
              perror (ANSI_COLOR_RED "> Can't get hub descriptor" ANSI_COLOR_RESET);
              usb_close (uh);
              continue;
            }
          }

          int port_count = buf[2];
          hubs[hub_count].busnum = atoi(bus->dirname);
          hubs[hub_count].devnum = dev->devnum;
          hubs[hub_count].dev = dev;
          hubs[hub_count].port_count = port_count;

          hub_count++;
          list_ports (uh, port_count); // print port status
          usb_close (uh);
      }
    }
  }

  if (hub_count == 0) {
    printf (ANSI_COLOR_RED "> No hub found.\n" ANSI_COLOR_RESET);
    exit(1);
  }
}

int get_hub (int busnum, int devnum) {
  for (int i = 0; i < hub_count; i++) {
    if (hubs[i].busnum == busnum && hubs[i].devnum == devnum) {
      return i;
    }
  }
  return -1;
}

int main (int argc, const char *argv[]) {
  int busnum = 0;
  int devnum = 0;
  int port = 0;
  int power = 1;
  int request = USB_REQ_CLEAR_FEATURE;
  int hub = -1;
  usb_dev_handle *uh = NULL;

  for (int i = 1; i < argc; i++) {
    if (argv[i][0] == '-') {
      switch (argv[i][1]) {
        case 'H':
          hub = atoi(argv[++i]);
          break;

        case 'B':
          if (hub == -1) {
            busnum = atoi(argv[++i]);
          }
          break;

        case 'D':
          if (hub == -1) {
            devnum = atoi(argv[++i]);
          }
          break;

        case 'P':
          port = atoi(argv[++i]);
          break;

        case 'p':
          power = atoi(argv[++i]);
          break;

        default:
          exit_with_usage (argv[0]);
      }
    }
    else {
      exit_with_usage (argv[0]);
    }
  }
  usb_init();
  usb_find_busses();
  usb_find_devices();

  list_hubs(port); // port is only used to decide to print details or not

  if (port == 0) {// no port number given, stop here
    exit (0);
  }

  if (hub < 0) {// no hub number given
    hub = get_hub(busnum, devnum);
  }

  if (hub >= 0 && hub < hub_count) {// valid hub number
    uh = usb_open (hubs[hub].dev);
  }

  if (uh == NULL) {
    printf (ANSI_COLOR_RED "> Device not found.\n" ANSI_COLOR_RESET);
    exit(1);
  }

  if (power > 0) {
    request = USB_REQ_SET_FEATURE;
  }

  if (usb_control_msg(uh, USB_RT_PORT, request, USB_PORT_FEAT_POWER, port, NULL, 0, CTRL_TIMEOUT) < 0) {
    printf (ANSI_COLOR_RED "> Failed to control.\n" ANSI_COLOR_RESET);
    exit(1);
  }

  list_ports(uh, hubs[hub].port_count);
  printf ("> Hub:%d Bus:%d Devive:%d Port:%d power->%d\n",hub, busnum, devnum, port, power);

  exit(0);
}
