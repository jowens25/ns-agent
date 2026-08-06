#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/route.h>
#include <unistd.h>
#include <sys/reboot.h>

#include <glib.h>
#include <NetworkManager.h>
#include <stdio.h>
#include <arpa/inet.h>
struct in_addr prefix_to_netmask(guint32 prefix)
{
    struct in_addr netmask;
    if (prefix == 0)
        netmask.s_addr = 0;
    else
        netmask.s_addr = htonl(~((1U << (32 - prefix)) - 1));
    return netmask;
}

void getIPInfoStr(char *dstBuff)
{

    struct ifaddrs *ifaddr, *ifa;
    int s;
    char host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) == -1)
    {
        return;
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == NULL)
            continue;

        s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

        if ((strcmp(ifa->ifa_name, "eth0") == 0) && (ifa->ifa_addr->sa_family == AF_INET))
        {
            if (s != 0)
            {
                return;
            }
            char mask[INET_ADDRSTRLEN];
            void *mask_ptr = &((struct sockaddr_in *)ifa->ifa_netmask)->sin_addr;
            inet_ntop(AF_INET, mask_ptr, mask, INET_ADDRSTRLEN);
            char buffTemp[24] = {0};
            memcpy(buffTemp, host, 19);
            snprintf(dstBuff, 29, "$IP:%s,", buffTemp);
            strncat(dstBuff, mask, 28);
            strncat(dstBuff, ",", 4);
        }
    }

    freeifaddrs(ifaddr);

    FILE *f;
    char line[100], *p, *c, *d;
    uint8_t gwFound = 0;
    f = fopen("/proc/net/route", "r");
    if (f != NULL)
    {

        while (fgets(line, 100, f))
        {

            p = strtok(line, " \t"); // interface
            c = strtok(NULL, " \t"); // destination
            d = strtok(NULL, " \t"); // gateway

            if (p != NULL && c != NULL && d != NULL)
            {
                if (strcmp(c, "00000000") == 0)
                {
                    char *gw;
                    char g[16] = {0};
                    memcpy(g, d + 6, 2);
                    memcpy(g + 2, d + 4, 2);
                    memcpy(g + 4, d + 2, 2);
                    memcpy(g + 6, d, 2);

                    struct in_addr addr;
                    addr.s_addr = htonl(strtoul(g, NULL, 16));
                    gw = inet_ntoa(addr);
                    if (gw != NULL)
                    {
                        strncat(dstBuff, gw, 20);
                        gwFound = 1;
                    }
                    break; // jo
                }
            }
        }
    }

    if (gwFound == 0) // we didn't get a gw in proc... meaning we are not connected...
    {
        NMClient *client;
        NMConnection *connection;
        NMSettingConnection *s_con;
        NMSettingWired *s_wired;
        NMSettingIPConfig *s_ip4;
        NMIPAddress *addr;
        NMDevice *device;
        GError *error = NULL;
        const GPtrArray *devices;
        GMainLoop *loop;
        // CallbackData callback_data;

        client = nm_client_new(NULL, &error);
        if (!client)
        {
            fprintf(stderr, "Failed to create NM client: %s\n", error->message);
            g_error_free(error);
            return 1;
        }

        device = NULL;
        devices = nm_client_get_devices(client);

        for (int i = 0; i < devices->len; i++)
        {
            NMDevice *dev = g_ptr_array_index(devices, i);
            if (NM_IS_DEVICE_ETHERNET(dev))
            {
                device = dev;
                printf("Found Ethernet device: %s\n", nm_device_get_iface(dev));
                break;
            }
        }

        if (!device)
        {
            fprintf(stderr, "No Ethernet device found\n");
            g_object_unref(client);
            return 1;
        }

        // Get IPv4 configuration object from the device

        NMConnection *conn = NULL;

        const char *iface = nm_device_get_iface(device);
        const GPtrArray *all_conns = nm_client_get_connections(client); // NMClient* you already have

        for (guint i = 0; i < all_conns->len; i++)
        {
            NMConnection *c = NM_CONNECTION(all_conns->pdata[i]);
            NMSettingConnection *s_con = nm_connection_get_setting_connection(c);
            if (!s_con)
                continue;

            const char *conn_iface = nm_setting_connection_get_interface_name(c ? s_con : NULL);
            if (conn_iface && g_strcmp0(conn_iface, iface) == 0)
            {
                conn = c;
                break;
            }
        }

        if (conn)
        {
            s_ip4 = nm_connection_get_setting_ip4_config(conn);
            const char *gateway = nm_setting_ip_config_get_gateway(s_ip4);
            NMIPAddress *addr = nm_setting_ip_config_get_address(s_ip4, 0);
            const char *ipaddress = nm_ip_address_get_address(addr);
            guint prefix = nm_ip_address_get_prefix(addr); // e.g. 24
            struct in_addr mask = prefix_to_netmask(prefix);

            // printf("Netmask: %s\n", inet_ntoa(mask)); // 255.255.255.0

            sprintf(dstBuff, "$IP:%s,%s,%s", ipaddress, inet_ntoa(mask), gateway);
        }
        else
        {
            strcat(dstBuff, "$IP:0.0.0.0,255.255.255.0,0.0.0.0");
        }
    }

    strncat(dstBuff, "\r\n\0", 4);

    fclose(f);
}

int main(void)
{

    char data[100] = {0};

    getIPInfoStr(data);

    printf("%s\n", data);
}