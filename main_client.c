#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include "client.h"
int main()
{
    printf("Establishing connection to server\n");
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd == -1)
    {
        perror("Error: ");
        return -1;
    }

    struct sockaddr_in serv;

    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = INADDR_ANY;
    serv.sin_port = htons(5555);

    if (connect(sockfd, (struct sockaddr *)&serv, sizeof(serv)) == -1)
    {
        perror("Error: ");
        return -1;
    }

    printf("Success\n");
    printf("1->Login as User\n");
    printf("2->Login as Admin\n");
    int user;
    scanf("%d", &user);
    write(sockfd, &user, sizeof(user));

    if (user == 1)
    {
        int cusid, res = -1;
        while (1)
        {
            printf("a->Signup as User\n");
            printf("b->Login as User\n");
            printf("c-> Exit\n");
            printf("Please enter a choice\n");
            char ch;
            scanf("%c", &ch);
            scanf("%c", &ch);
            write(sockfd, &ch, sizeof(char));
            if (ch == 'a')
            {
                AddNewCustomer(sockfd);
            }
            else if (ch == 'b')
            {
                while (1)
                {
                    printf("Enter customer id\n");
                    scanf("%d", &cusid);

                    if (cusid < 0)
                    {
                        printf("Customer id can't be negative, try again\n");
                    }
                    else
                    {
                        break;
                    }
                }
                printf("%d\n",cusid);
                write(sockfd, &cusid, sizeof(int));
                read(sockfd, &res, sizeof(int));
                if (res == -1)
                {
                    printf("Invalid Login.Please Try again\n");
                }
                if (res != -1)
                {
                    break;
                }
            }
            if (ch == 'c')
            {
                exit(0);
            }
        }
        while (1)
        {
            displayMenuUser();
            char ch;
            scanf("%c", &ch);
            scanf("%c", &ch);

            write(sockfd, &ch, sizeof(char));

            if (ch == 'a')
            {
                break;
            }
            else if (ch == 'b')
            {
                getInventory(sockfd);
            }
            else if (ch == 'c')
            {
                GetCartItems(sockfd, cusid);
            }
            else if (ch == 'd')
            {
                write(sockfd, &cusid, sizeof(int));
                AddProductToCart(sockfd);
            }
            else if (ch == 'e')
            {
                write(sockfd, &cusid, sizeof(int));
                EditProductInCart(sockfd);
            }
            else if (ch == 'f')
            {
                write(sockfd, &cusid, sizeof(int));
                ProceedToPayment(sockfd);
            }
            else
            {
                printf("Invalid choice, try again\n");
            }
        }
    }
    else if (user == 2)
    {

        while (1)
        {
            displayMenuAdmin();
            char ch;
            scanf("%c", &ch);
            scanf("%c", &ch);
            write(sockfd, &ch, sizeof(ch));

            if (ch == 'a')
            {
                AddProduct(sockfd);
            }

            else if (ch == 'b')
            {
                DeleteAProduct(sockfd);
            }

            else if (ch == 'c')
            {
                UpdateProduct(sockfd, 0); // Updating price
            }

            else if (ch == 'd')
            {
                UpdateProduct(sockfd, 1); // U[dating quantity
            }

            else if (ch == 'e')
            {
                getInventory(sockfd);
            }

            else if (ch == 'f')
            {
                break;
            }

            else
            {
                printf("Invalid choice, try again\n");
            }
        }
    }

    printf("Exiting program\n");
    close(sockfd);
    return 0;
}