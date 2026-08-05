// #include <arpa/inet.h>
// #include <sys/socket.h>
// #include <netdb.h>
// #include <ifaddrs.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <unistd.h>
// #include <string.h>
// #include <sys/ioctl.h>
// #include <net/if.h>
// #include <linux/route.h>
// #include <unistd.h>
// #include <sys/reboot.h>

// #include <glib.h>
// #include <NetworkManager.h>
// #include <stdio.h>
// #include <arpa/inet.h>

// void getIPInfoStr(char *dstBuff)
// {

//     struct ifaddrs *ifaddr, *ifa;
//     int s;
//     char host[NI_MAXHOST];

//     if (getifaddrs(&ifaddr) == -1)
//     {
//         return;
//     }

//     for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
//     {
//         if (ifa->ifa_addr == NULL)
//             continue;

//         s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

//         if ((strcmp(ifa->ifa_name, "enp4s0") == 0) && (ifa->ifa_addr->sa_family == AF_INET))
//         {
//             if (s != 0)
//             {
//                 return;
//             }
//             char mask[INET_ADDRSTRLEN];
//             void *mask_ptr = &((struct sockaddr_in *)ifa->ifa_netmask)->sin_addr;
//             inet_ntop(AF_INET, mask_ptr, mask, INET_ADDRSTRLEN);
//             char buffTemp[24] = {0};
//             memcpy(buffTemp, host, 19);
//             snprintf(dstBuff, 29, "$IP:%s,", buffTemp);
//             strncat(dstBuff, mask, 28);
//             strncat(dstBuff, ",", 4);
//         }
//     }

//     freeifaddrs(ifaddr);

//     FILE *f;
//     char line[100], *p, *c, *d;
//     uint8_t gwFound = 0;
//     f = fopen("/proc/net/route", "r");
//     if (f != NULL)
//     {

//         while (fgets(line, 100, f))
//         {

//             p = strtok(line, " \t"); // interface
//             c = strtok(NULL, " \t"); // destination
//             d = strtok(NULL, " \t"); // gateway

//             if (p != NULL && c != NULL && d != NULL)
//             {
//                 if (strcmp(c, "00000000") == 0)
//                 {
//                     char *gw;
//                     char g[16] = {0};
//                     memcpy(g, d + 6, 2);
//                     memcpy(g + 2, d + 4, 2);
//                     memcpy(g + 4, d + 2, 2);
//                     memcpy(g + 6, d, 2);

//                     struct in_addr addr;
//                     addr.s_addr = htonl(strtoul(g, NULL, 16));
//                     gw = inet_ntoa(addr);
//                     if (gw != NULL)
//                     {
//                         strncat(dstBuff, gw, 20);
//                         gwFound = 1;
//                     }
//                     break; // jo
//                 }
//             }
//         }
//     }

//     if (gwFound == 0) // we didn't get a gw in proc... meaning we are not connected...
//     {
//         NMClient *client;
//         NMConnection *connection;
//         NMSettingConnection *s_con;
//         NMSettingWired *s_wired;
//         NMSettingIPConfig *s_ip4;
//         NMIPAddress *addr;
//         NMDevice *device;
//         GError *error = NULL;
//         const GPtrArray *devices;
//         GMainLoop *loop;
//         // CallbackData callback_data;

//         client = nm_client_new(NULL, &error);
//         if (!client)
//         {
//             fprintf(stderr, "Failed to create NM client: %s\n", error->message);
//             g_error_free(error);
//             return 1;
//         }

//         device = NULL;
//         devices = nm_client_get_devices(client);

//         for (int i = 0; i < devices->len; i++)
//         {
//             NMDevice *dev = g_ptr_array_index(devices, i);
//             if (NM_IS_DEVICE_ETHERNET(dev))
//             {
//                 device = dev;
//                 printf("Found Ethernet device: %s\n", nm_device_get_iface(dev));
//                 break;
//             }
//         }

//         if (!device)
//         {
//             fprintf(stderr, "No Ethernet device found\n");
//             g_object_unref(client);
//             return 1;
//         }

//         // Get IPv4 configuration object from the device

//         const GPtrArray *conns = nm_device_get_available_connections(device);
//         if (conns && conns->len > 0)
//         {
//             NMConnection *conn = NM_CONNECTION(conns->pdata[0]); // pick the right one if there are several
//             s_ip4 = nm_connection_get_setting_ip4_config(conn);
//             const char *gateway = nm_setting_ip_config_get_gateway(s_ip4);

//             strcat(dstBuff, gateway);
//         }
//         else
//         {
//             strcat(dstBuff, "0.0.0.0");
//         }
//     }

//     strncat(dstBuff, "\r\n\0", 4);

//     fclose(f);
// }

// int main(void)
// {
//     char data[100] = {0};
//     getIPInfoStr(data);

//     printf("%s\n", data);
// }