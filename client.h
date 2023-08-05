#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "headers.h"
#include <fcntl.h>
#include <sys/stat.h>

void displayMenuUser();
void displayMenuAdmin();
void getInventory(int sockfd);
int calculateTotal(struct cart c);
void generateReceipt(int total, struct cart c, int sockfd);
int prodIdTaker();
int priceTaker();
int quantityTaker();
void GetCartItems(int sockfd,int cusid);
void AddProductToCart(int sockfd);
void EditProductInCart(int sockfd);
void ProceedToPayment(int sockfd);
void AddNewCustomer(int sockfd);
void AddProduct(int sockfd);
void DeleteAProduct(int sockfd);
void UpdateProduct(int sockfd,int pri_or_quan);