#include <stdio.h>
#include <string.h>
#include <stdlib.h>
int main()
{
    float        gps_latitude;
    char         gps_latitude_chr;
    float        gps_longitude;
    char         gps_longitude_chr;
    unsigned int gps_sv_number;
    float        gps_hdop;
    int          gps_valid;

    char *msg_bufp = malloc(256);
    strcpy(msg_bufp, "$GPGGA,092725.00,4717.11399,N,00833.91590,E,1,08,1.01,499.6,M,48.0,M,,*5B\r\n");
    char* message_line = strsep(&msg_bufp, "\r\n");
    while (message_line != NULL)
    {
        unsigned int message_checksum = 0;
        for (size_t i = 0; i < strlen(message_line); i++)
        {
            if (message_line[i] == '$')
            {
                continue;
            }
            else if (message_line[i] == '*')
            {
                message_line[i] = ',';
                break;
            }
            message_checksum ^= message_line[i];
        }
        // -1   $xxGGA
        // 0    Time (Not Used)
        // 1    Latitude
        // 2    N/S
        // 3    Longitude
        // 4    E/W
        // 5    Quality
        // 6    SvNumber
        // 7    HDOP
        // 8    Altitude (Not Used)
        // 9    "M"
        // 10   Separation (Not Used)
        // 11   "M"
        // 12   DifferentialAge (Not Used)
        // 13   DifferentialStation (Not Used)
        // 14   Checksum
        char* message_id = strsep(&message_line, ",");
        if (strcmp(message_id + 3, "GGA") != 0)
        {
            goto message_line_next;
        }
        else
        {
            const size_t entry_count = 15;
            char*        entry_list[entry_count];
            for (size_t i = 0; i < entry_count ; i++)
            {
                entry_list[i] = strsep(&message_line, ",");
            }
            // checksum
            unsigned int checksum_l4b[2] = { (message_checksum & 0xF0) >> 4, message_checksum & 0x0F };
            char         checksum_chr[2] = {
                (char)((checksum_l4b[0] <= 0x9) ? (checksum_l4b[0] + '0') : (checksum_l4b[0] - 10 + 'A')),
                (char)((checksum_l4b[1] <= 0x9) ? (checksum_l4b[1] + '0') : (checksum_l4b[1] - 10 + 'A'))
            };
            if (entry_list[14][0] != checksum_chr[0] || entry_list[14][1] != checksum_chr[1])
            {
                printf("gps: crtical warning! a checksum error has been detected, skipped.\n");
                goto message_line_next;
            }
            else
            {
                // time unused
                (void)entry_list[0];
                // latitude parser
                unsigned int latitude_degree;
                float        latitude_minute;
                sscanf(entry_list[1], "%2u%f", &latitude_degree, &latitude_minute);
                gps_latitude     = latitude_degree + latitude_minute / 60.0f;
                gps_latitude_chr = entry_list[2][0];
                // longitiude parser
                unsigned int longitude_degree;
                float        longitude_minute;
                sscanf(entry_list[3], "%3u%f", &longitude_degree, &longitude_minute);
                gps_longitude     = longitude_degree + longitude_minute / 60.0f;
                gps_longitude_chr = entry_list[4][0];
                // quality parser
                unsigned int quality;
                sscanf(entry_list[5], "%u", &quality);
                if (quality == 0 || quality == 6)
                {
                    gps_valid = 0;
                }
                else if (gps_valid == 0)
                {
                    printf("gps: first fix captured\n");
                    printf(
                        "gps: pos: %2.9f%c,%2.9f%c\n", gps_latitude, gps_latitude_chr, gps_longitude, gps_longitude_chr);
                    gps_valid = 1;
                }
                else
                {
                    gps_valid = 1;
                }
                // svnumber parser
                sscanf(entry_list[6], "%u", &(gps_sv_number));
                // HDOP
                sscanf(entry_list[7], "%f", &(gps_hdop));

                (void)entry_list[8];
                (void)entry_list[9];
                (void)entry_list[10];
                (void)entry_list[11];
                (void)entry_list[12];
                (void)entry_list[13];
            }
        }
message_line_next:
        message_line = strsep(&msg_bufp, "\r\n");
        continue;
    }

    // float        gps_latitude;
    //     char         gps_latitude_chr;
    //     float        gps_longitude;
    //     char         gps_longitude_chr;
    //     unsigned int gps_sv_number;
    //     float        gps_hdop;
    //     int          gps_valid;
    //     char         gps_mode;
    printf(
        "%f,%c,%f,%c,%u,%f,%d\n",
        gps_latitude,
        gps_latitude_chr,
        gps_longitude,
        gps_longitude_chr,
        gps_sv_number,
        gps_hdop,
        gps_valid);
}
