#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <stdbool.h>
#include "headers.h"

void SetLockCustom(int fd_custs, struct flock lock_cust);
void unlock(int fd, struct flock lock);
void productReadLock(int fd, struct flock lock);
void productWriteLock(int fd, struct flock lock);
void cartOffsetLock(int fd_cart, struct flock lock_cart, int offset, int ch);
int OffSetGetter(int cust_id, int fd_custs);
void addProducts(int fd, int new_fd, int fd_admin);
void listProducts(int fd, int new_fd);
void deleteProduct(int fd, int new_fd, int id, int fd_admin);
void updateProduct(int fd, int new_fd, int ch, int fd_admin);
void addCustomer(int fd_cart, int fd_custs, int new_fd);
void viewCart(int fd_cart, int new_fd, int fd_custs);
void addProductToCart(int fd, int fd_cart, int fd_custs, int new_fd);
void editProductInCart(int fd, int fd_cart, int fd_custs, int new_fd);
void payment(int fd, int fd_cart, int fd_custs, int new_fd);
void generateAdminReceipt(int fd_admin, int fd);
bool checkQuantity(int fd_items, struct product p);