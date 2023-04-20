#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "ride.h"
#define TABLE_SIZE 1200

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>

int main()
{
    // open files
    FILE *bfp = fopen("data/rides.bin", "rb");
    FILE *source_id_table_file = fopen("data/source_id_table.bin", "rb");
    if (bfp == NULL || source_id_table_file == NULL)
    {
        printf("Can't open files");
        exit(-1);
    }

    // initialize table
    int source_id_table[TABLE_SIZE];
    for (int i = 0; i < 1200; i++)
    {
        source_id_table[i] = -1;
    }

    // load source id table
    fread(&source_id_table, sizeof(source_id_table), 1, source_id_table_file);

    // search variables
    int source_id;
    int dest_id;
    int hour;
    float avg_travel_time;
    // searching variables
    int infile_pos;
    bool found;
    // fifo variables
    int arrArrival[3];
    int fd;

    // create pipe
    if (mkfifo("/tmp/myfifo", 0777) == -1)
    {
        if (errno != EEXIST)
        {
            printf("Could not create fifo file.\n");
            return 1;
        }
    }

    while (true)
    {
        // receives data from user interface process

        fd = open("/tmp/myfifo", O_RDONLY); // open pipe for reading
        if (fd == -1)
        {
            printf("Can't open pipe from reading.\n");
            return 1;
        }
        if (read(fd, &arrArrival, sizeof(arrArrival)) == -1) // read attributes to arrArrival array
        {
            printf("Can't read from pipe.\n");
            return 2;
        }
        close(fd); // close pipe

        if (arrArrival[0] == -1) // program termination flag
        {
            // close files
            fclose(bfp);
            fclose(source_id_table_file);
            return 0;
        }

        // save arrival data on corresponding variables
        source_id = arrArrival[0];
        dest_id = arrArrival[1];
        hour = arrArrival[2];
        Ride *ride = malloc(sizeof(Ride)); // allocate memory for Ride struct

        // start search procedure
        if (source_id_table[source_id] == -1) // no rides with that source id
        {
            avg_travel_time = -1.0;        // NA flag
            fd = open("/tmp/myfifo", O_WRONLY); // open pipe
            if (write(fd, &avg_travel_time, sizeof(float)) == -1)
            {
                printf("Can't write in pipe.");
                return 2;
            }
            close(fd); // close pipe
        }
        else
        {
            infile_pos = source_id_table[source_id];
            found = false;
            do // search through the linked list of the source ID
            {
                fseek(bfp, infile_pos, SEEK_SET);
                fread(ride, sizeof(Ride), 1, bfp);
                if (ride->hour == hour && ride->dest_id == dest_id) // checks criteria
                {
                    avg_travel_time = ride->avg_time; // save average travel time
                    fd = open("/tmp/myfifo", O_WRONLY);    // open pipe
                    if (write(fd, &avg_travel_time, sizeof(float)) == -1)
                    {
                        printf("Can't write in pipe.\n");
                        return 2;
                    }
                    close(fd); // close pipe
                    found = true;
                    break;
                }
                infile_pos = ride->next_source_id;

            } while (infile_pos != -1);

            if (!found)
            {
                avg_travel_time = -1.0;        // NA flag
                fd = open("/tmp/myfifo", O_WRONLY); // open pipe
                if (write(fd, &avg_travel_time, sizeof(float)) == -1)
                {
                    printf("Can't write in pipe.\n");
                    return 2;
                }
                close(fd); // close pipe
            }
        }
        free(ride); // free memory from ride struct
    }
}
