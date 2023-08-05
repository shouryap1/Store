#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include "server.h"

int main()
{
    printf("Setting up server\n");

    // file containing all the records is called records.txt

    int fd = open("records.txt", O_RDWR | O_CREAT, 0777);//
    int fd_cart = open("orders.txt", O_RDWR | O_CREAT, 0777);
    int fd_custs = open("customers.txt", O_RDWR | O_CREAT, 0777);
    int fd_admin = open("adminReceipt.txt", O_RDWR | O_CREAT, 0777);
    lseek(fd_admin, 0, SEEK_END);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd == -1)
    {
        perror("Error: ");
        return -1;
    }

    struct sockaddr_in serv, cli;

    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = INADDR_ANY;
    serv.sin_port = htons(5555);

    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
    {
        perror("Error: ");
        return -1;
    }

    if (bind(sockfd, (struct sockaddr *)&serv, sizeof(serv)) == -1)
    {
        perror("Error: ");
        return -1;
    }

    if (listen(sockfd, 5) == -1)
    {
        perror("Error: ");
        return -1;
    }

    socklen_t size = sizeof(cli);
    printf("Server set up successfully\n");

    while (1)
    {

        int new_fd = accept(sockfd, (struct sockaddr *)&cli, &size);
        if (new_fd == -1)
        {
            return -1;
        }

        if (!fork())
        {
            printf("Connection with client successful\n");
            close(sockfd);

            int user;
            read(new_fd, &user, sizeof(int));

            if (user == 1)
            {
                int offset=-1;
                while(1)
                {
                    char login_or_signup;
                    read(new_fd, &login_or_signup, sizeof(char));
                    if (login_or_signup == 'a')
                    {
                        addCustomer(fd_cart, fd_custs, new_fd);
                    }
                    else if (login_or_signup == 'b')
                    {
                        int customerID;
                        read(new_fd, &customerID, sizeof(int));
                        offset = OffSetGetter(customerID, fd_custs);
                        write(new_fd, &offset, sizeof(int));
                        if (offset !=-1)
                        {
                            break;
                        }
                    }
                }
                char ch;
                while (1)
                {
                    read(new_fd, &ch, sizeof(char));

                    lseek(fd, 0, SEEK_SET);
                    lseek(fd_cart, 0, SEEK_SET);
                    lseek(fd_custs, 0, SEEK_SET);

                    if (ch == 'a')
                    {
                        close(new_fd);
                        break;
                    }
                    else if (ch == 'b')
                    {
                        listProducts(fd, new_fd);
                    }

                    else if (ch == 'c')
                    {
                        viewCart(fd_cart, new_fd, fd_custs);
                    }

                    else if (ch == 'd')
                    {
                        addProductToCart(fd, fd_cart, fd_custs, new_fd);
                    }

                    else if (ch == 'e')
                    {
                        editProductInCart(fd, fd_cart, fd_custs, new_fd);
                    }

                    else if (ch == 'f')
                    {
                        payment(fd, fd_cart, fd_custs, new_fd);
                    }
                }
                printf("Connection terminated\n");
            }
            else if (user == 2)
            {
                char ch;
                while (1)
                {
                    read(new_fd, &ch, sizeof(ch));

                    lseek(fd, 0, SEEK_SET);
                    lseek(fd_cart, 0, SEEK_SET);
                    lseek(fd_custs, 0, SEEK_SET);

                    if (ch == 'a')
                    {
                        addProducts(fd, new_fd, fd_admin);
                    }
                    else if (ch == 'b')
                    {
                        int id;
                        read(new_fd, &id, sizeof(int));
                        deleteProduct(fd, new_fd, id, fd_admin);
                    }
                    else if (ch == 'c')
                    {
                        updateProduct(fd, new_fd, 1, fd_admin);
                    }

                    else if (ch == 'd')
                    {
                        updateProduct(fd, new_fd, 2, fd_admin);
                    }

                    else if (ch == 'e')
                    {
                        listProducts(fd, new_fd);
                    }

                    else if (ch == 'f')
                    {
                        close(new_fd);
                        generateAdminReceipt(fd_admin, fd);
                        break;
                    }
                    else
                    {
                        continue;
                    }
                }
            }
            printf("Connection terminated\n");
        }
        else
        {
            close(new_fd);
        }
    }
}