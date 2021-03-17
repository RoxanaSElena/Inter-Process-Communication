#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <math.h>

#define RESP_PIPE "RESP_PIPE_59074"
#define REQ_PIPE "REQ_PIPE_59074"

typedef struct __attribute__((packed))
{
    char sect_name[16];
    char sect_type;
    int sect_offset;
    int sect_size;
} sections;

int main(void)
{
    int fd1 = -1;
    int fd2 = -1;
    char c[] = "CONNECT";
    int len_c = strlen(c);
    char *str = (char *)malloc(50 * sizeof(char));
    int len_str = strlen(str);
    char pong[] = "PONG";
    int len_pong = strlen(pong);
    int var = 59074;
    int nr;
    char success[] = "SUCCESS";
    int len_success = strlen(success);
    char error[] = "ERROR";
    int len_error = strlen(error);
    char *p;
    char file_name[256];
    int fd_map;
    char *data = NULL;
    off_t size;
    short header_size;
    char version;
    unsigned char nb_sections;
    int align = 2048;

    //if there is already the pipe, delete it
    if (access(RESP_PIPE, 0) == 0)
    {
        unlink(RESP_PIPE);
    }

    //creeaza RESP_PIPE
    if (mkfifo(RESP_PIPE, 0600) != 0)
    {
        // perror("error creating the pipe1");
        printf("ERROR\n cannot create the response pipe\n");
        return 1;
    }

    //open REQ_PIPE

    fd2 = open(REQ_PIPE, O_RDONLY);
    if (fd2 == -1)
    {
        printf("ERROR\n cannot open the request pipe\n");
        return 1;
    }

    //open RESP_PIPE

    fd1 = open(RESP_PIPE, O_WRONLY);
    if (fd1 == -1)
    {
        printf("ERROR\n cannot open the response pipe\n");
        return 1;
    }

    write(fd1, &len_c, sizeof(char));
    write(fd1, c, len_c);

    int we_read = 1;
    // printf("SUCCESS\n");

    while (we_read)
    {
        read(fd2, &len_str, sizeof(char));
        read(fd2, str, len_str);
        str[len_str] = '\0';

        if (strcmp(str, "EXIT") == 0) //condition to get out of the loop
        {
            we_read = 0;
        }
        //2.3 REQUEST PING
        else if (strcmp(str, "PING") == 0)
        {
            write(fd1, &len_str, sizeof(char));
            write(fd1, str, len_str);
            write(fd1, &len_pong, sizeof(char));
            write(fd1, pong, len_pong);
            write(fd1, &var, sizeof(int));
            write(fd1, &len_success, sizeof(char));
            write(fd1, success, len_success);
        }
        else
        {
            //2.4 CREATE A MEMORY SHARED REGION
            if (strcmp(str, "CREATE_SHM") == 0)
            {
                read(fd2, &nr, sizeof(int));
                int shm_fd;
                shm_fd = shm_open("/B8gXSYXI", O_CREAT | O_RDWR, 0644);
                if (shm_fd == -1)
                {
                    write(fd1, &len_str, sizeof(char));
                    write(fd1, str, len_str);
                    write(fd1, &len_error, sizeof(char));
                    write(fd1, error, len_error);
                }
                ftruncate(shm_fd, nr);
                //mapare
                p = mmap(0, nr, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
                if (p == MAP_FAILED)
                {
                    write(fd1, &len_str, sizeof(char));
                    write(fd1, str, len_str);
                    write(fd1, &len_error, sizeof(char));
                    write(fd1, error, len_error);
                }
                write(fd1, &len_str, sizeof(char));
                write(fd1, str, len_str);
                write(fd1, &len_success, sizeof(char));
                write(fd1, success, len_success);
            }
            //2.5 WRITE TO THE SHARED MEMORY
            else if (strcmp(str, "WRITE_TO_SHM") == 0)
            {
                unsigned int offset;
                unsigned int value;
                read(fd2, &offset, sizeof(int)); //read the offset from where we write
                read(fd2, &value, sizeof(int));  //read how many bytes we have to write
                if (offset + 4 < nr)             // +4 because of unsigned int
                {

                    memcpy(&p[offset], &value, sizeof(int));
                    write(fd1, &len_str, sizeof(char));
                    write(fd1, str, len_str);
                    write(fd1, &len_success, sizeof(char));
                    write(fd1, success, len_success);
                }
                else
                {
                    write(fd1, &len_str, sizeof(char));
                    write(fd1, str, len_str);
                    write(fd1, &len_error, sizeof(char));
                    write(fd1, error, len_error);
                }
            }
            //2.6 MEMORY-MAP FILE REQUEST

            else if (strcmp(str, "MAP_FILE") == 0)
            {

                char file_size = 0;                  //size-ul unui fisier este in bytes
                read(fd2, &file_size, sizeof(char)); //read the size of the file
                read(fd2, file_name, file_size);     //read the name of the file

                fd_map = open(file_name, O_RDONLY);
                if (fd_map == -1) //if cannot open
                {
                    write(fd1, &len_str, sizeof(char));
                    write(fd1, str, len_str);
                    write(fd1, &len_error, sizeof(char));
                    write(fd1, error, len_error);
                }
                else
                {

                    size = lseek(fd_map, 0, SEEK_END); //find the size of our file
                    lseek(fd_map, 0, SEEK_SET);        //go back to the beginning

                    data = (char *)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd_map, 0); // maparea fisierului in memorie II UN ARRAY AL FISIERULUI

                    if (data == (void *)-1)
                    {
                        write(fd1, &len_str, sizeof(char));
                        write(fd1, str, len_str);
                        write(fd1, &len_error, sizeof(char));
                        write(fd1, error, len_error);
                    }
                    else
                    {

                        write(fd1, &len_str, sizeof(char));
                        write(fd1, str, len_str);
                        write(fd1, &len_success, sizeof(char));
                        write(fd1, success, len_success);
                    }
                }
            }
            //2.7 READ FROM FILE OFFSET
            else if (strcmp(str, "READ_FROM_FILE_OFFSET") == 0)
            {
                int offset;
                int no_of_bytes;

                read(fd2, &offset, sizeof(int));      //read the offset
                read(fd2, &no_of_bytes, sizeof(int)); // read nr of bytes

                if (offset + no_of_bytes < size)
                {
                    memcpy(p, &data[offset], no_of_bytes);
                    write(fd1, &len_str, sizeof(char));
                    write(fd1, str, len_str);
                    write(fd1, &len_success, sizeof(char));
                    write(fd1, success, len_success);
                }
                else
                {
                    write(fd1, &len_str, sizeof(char));
                    write(fd1, str, len_str);
                    write(fd1, &len_error, sizeof(char));
                    write(fd1, error, len_error);
                }
            }
            else

                //2.8 READ FROM FILE SECTION

                if (strncmp(str, "READ_FROM_FILE_SECTION", 22) == 0)
            {

                int section_no;
                int offset;
                int no_of_bytes;

                //STIU SIZE -cat de mare-i fisierul

                read(fd2, &section_no, sizeof(int));  //read the section_no
                read(fd2, &offset, sizeof(int));      //read the offset
                read(fd2, &no_of_bytes, sizeof(int)); //read the no_of_bytes

                memcpy(&header_size, data + size - 6, sizeof(short));
                memcpy(&version, data + size - header_size, sizeof(char));
                if (version < 43 || version > 73)
                {
                    
                    write(fd1, &len_str, sizeof(char));
                    write(fd1, str, len_str);
                    write(fd1, &len_error, sizeof(char));
                    write(fd1, error, len_error);
                }
                memcpy(&nb_sections, data + size - header_size + 1, sizeof(char));
                if (nb_sections < 5 || nb_sections > 14)
                {
                    
                    write(fd1, &len_str, sizeof(char));
                    write(fd1, str, len_str);
                    write(fd1, &len_error, sizeof(char));
                    write(fd1, error, len_error);
                }

                if (section_no > nb_sections)
                {
                    printf("%d %d", nb_sections, section_no);
                    write(fd1, &len_str, sizeof(char));
                    write(fd1, str, len_str);
                    write(fd1, &len_error, sizeof(char));
                    write(fd1, error, len_error);
                }
                else
                {
                    sections *fileSections = (sections *)malloc(nb_sections * sizeof(sections));

                    for (int i = 0; i < nb_sections; i++)
                    {
                        memcpy(&fileSections[i].sect_name, data + size - header_size + 2 + i * 25, 16);
                        memcpy(&fileSections[i].sect_type, data + size - header_size + 2 + i * 25 + 16, 1);
                        memcpy(&fileSections[i].sect_offset, data + size - header_size + 2 + i * 25 + 16 + 1, 4);
                        memcpy(&fileSections[i].sect_size, data + size - header_size + 2 + i * 25 + 16 + 1 + 4, 4);

                        if (!((fileSections[i].sect_type == 72) || (fileSections[i].sect_type == 76 || (fileSections[i].sect_type == 27) || (fileSections[i].sect_type == 69) || (fileSections[i].sect_type == 79))))
                        {

                        
                            write(fd1, &len_str, sizeof(char));
                            write(fd1, str, len_str);
                            write(fd1, &len_error, sizeof(char));
                            write(fd1, error, len_error);
                        }
                        if (section_no - 1 == i)
                        {
                            memcpy(p, data + offset + fileSections[i].sect_offset, no_of_bytes);

                            write(fd1, &len_str, sizeof(char));
                            write(fd1, str, len_str);
                            write(fd1, &len_success, sizeof(char));
                            write(fd1, success, len_success);
                        }
                    }
                }
            }

            else
            {

                //2.9 READ FROM LOGICAL OFFSET
                if (strcmp(str, "READ_FROM_LOGICAL_SPACE_OFFSET") == 0)
                {
                    int logical_offset;
                    int no_of_bytes;

                    read(fd2, &logical_offset, sizeof(int));
                    read(fd2, &no_of_bytes, sizeof(int));

                    memcpy(&header_size, data + size - 6, sizeof(short));
                    memcpy(&version, data + size - header_size, sizeof(char));
                    if (version < 43 || version > 73)
                    {

                        
                        write(fd1, &len_str, sizeof(char));
                        write(fd1, str, len_str);
                        write(fd1, &len_error, sizeof(char));
                        write(fd1, error, len_error);
                    }
                    memcpy(&nb_sections, data + size - header_size + 1, sizeof(char));
                    if (nb_sections < 5 || nb_sections > 14)
                    {
                        
                        write(fd1, &len_str, sizeof(char));
                        write(fd1, str, len_str);
                        write(fd1, &len_error, sizeof(char));
                        write(fd1, error, len_error);
                    }

                    sections *fileSections = (sections *)malloc(nb_sections * sizeof(sections));
                    int *offset_vector = (int *)calloc(nb_sections, sizeof(int));
                    int crt_offset = 0;

                    for (int i = 0; i < nb_sections; i++)
                    {
                        memcpy(&fileSections[i].sect_name, data + size - header_size + 2 + i * 25, 16);
                        memcpy(&fileSections[i].sect_type, data + size - header_size + 2 + i * 25 + 16, 1);
                        memcpy(&fileSections[i].sect_offset, data + size - header_size + 2 + i * 25 + 16 + 1, 4);
                        memcpy(&fileSections[i].sect_size, data + size - header_size + 2 + i * 25 + 16 + 1 + 4, 4);

                        if (!((fileSections[i].sect_type == 72) || (fileSections[i].sect_type == 76 || (fileSections[i].sect_type == 27) || (fileSections[i].sect_type == 69) || (fileSections[i].sect_type == 79))))
                        {

                            
                            write(fd1, &len_str, sizeof(char));
                            write(fd1, str, len_str);
                            write(fd1, &len_error, sizeof(char));
                            write(fd1, error, len_error);
                        }

                        offset_vector[i] = crt_offset;

                        crt_offset += floor((fileSections[i].sect_size + align - 1) / align) * align; //math formula to calculate new offset

                        // printf("%d %d\n", fileSections[i].sect_size, offset_vector[i]);
                    }

                    int section_no = 0;

                    for (int i = 0; i < nb_sections - 1; i++) //cel mai mare numar mai mic decat logical_offset
                    {
                        if (offset_vector[i + 1] > logical_offset)
                        {
                            break;
                        }
                        section_no++;
                    }

                    memcpy(p, data + fileSections[section_no].sect_offset + logical_offset - offset_vector[section_no], no_of_bytes);
                    write(fd1, &len_str, sizeof(char));
                    write(fd1, str, len_str);
                    write(fd1, &len_success, sizeof(char));
                    write(fd1, success, len_success);
                }
            }
        }
    }

    close(fd1);
    close(fd2);

    return 0;
}