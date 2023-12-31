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
#include "server.h"

void SetLockCustom(int fd_custs, struct flock lock_cust)
{

    lock_cust.l_len = 0;
    lock_cust.l_type = F_RDLCK;
    lock_cust.l_start = 0;
    lock_cust.l_whence = SEEK_SET;
    fcntl(fd_custs, F_SETLKW, &lock_cust);
    return;
}

void unlock(int fd, struct flock lock)
{
    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLKW, &lock);
}

void productReadLock(int fd, struct flock lock)
{
    lock.l_len = 0;
    lock.l_type = F_RDLCK;
    lock.l_start = 0;
    lock.l_whence = SEEK_SET;
    fcntl(fd, F_SETLKW, &lock);
}

void productWriteLock(int fd, struct flock lock)
{
    lseek(fd, (-1) * sizeof(struct product), SEEK_CUR);
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_CUR;
    lock.l_start = 0;
    lock.l_len = sizeof(struct product);

    fcntl(fd, F_SETLKW, &lock);
}

void cartOffsetLock(int fd_cart, struct flock lock_cart, int offset, int ch)
{
    lock_cart.l_whence = SEEK_SET;
    lock_cart.l_len = sizeof(struct cart);
    lock_cart.l_start = offset;
    if (ch == 1)
    {
        // rdlck
        lock_cart.l_type = F_RDLCK;
    }
    else
    {
        // wrlck
        lock_cart.l_type = F_WRLCK;
    }
    fcntl(fd_cart, F_SETLKW, &lock_cart);
    lseek(fd_cart, offset, SEEK_SET);
}

int OffSetGetter(int cust_id, int fd_custs)
{
    if (cust_id < 0)
    {
        return -1;
    }
    struct flock lock_cust;
    SetLockCustom(fd_custs, lock_cust);
    struct index id;
    lseek(fd_custs,0,SEEK_SET);
    while (read(fd_custs, &id, sizeof(struct index)))
    {
        if (id.custid == cust_id)
        {
            unlock(fd_custs, lock_cust);
            return id.offset;
        }
    }
    unlock(fd_custs, lock_cust);
    return -1;
}
int NextProductID_Getter(int fd, int new_fd, int fd_admin)
{
    struct flock lock;
    productReadLock(fd, lock);
    int max_id = -1;
    int flag=0;
    struct product p;
    lseek(fd,0,SEEK_SET);
    while (read(fd, &p, sizeof(struct product)))
    {
        if (p.id > max_id)
        {
            flag=1;
            max_id = p.id;
        }
    }
    max_id++;
    if (flag==0)
    {
        max_id=1;
    }
    return max_id;
}
void addProducts(int fd, int new_fd, int fd_admin)
{

    char name[50];
    char response[100];
    int id, qty, price;

    struct product p1;
    int ProductId = NextProductID_Getter(fd, new_fd, fd_admin);
    write(new_fd,&ProductId,sizeof(int));
    int n = read(new_fd, &p1, sizeof(struct product));

    strcpy(name, p1.name);
    id = p1.id;
    qty = p1.qty;
    price = p1.price;

    struct flock lock;
    productReadLock(fd, lock);

    struct product p;

    int flg = 0;
    while (read(fd, &p, sizeof(struct product)))
    {

        if (p.id == id && p.qty > 0)
        {
            write(new_fd, "Duplicate product\n", sizeof("Duplicate product\n"));
            sprintf(response, "Adding product with product id %d failed as product id is duplicate\n", id);
            write(fd_admin, response, strlen(response));
            unlock(fd, lock);
            flg = 1;
            return;
        }
    }

    if (!flg)
    {

        // lseek(fd, 0, SEEK_END);
        lseek(fd,(ProductId-1)*sizeof(struct product),SEEK_SET);
        p.id = id;
        strcpy(p.name, name);
        p.price = price;
        p.qty = qty;

        write(fd, &p, sizeof(struct product));
        write(new_fd, "Added successfully\n", sizeof("Added succesfully\n"));
        sprintf(response, "New product with product id %d added successfully\n", id);
        write(fd_admin, response, strlen(response));
        unlock(fd, lock);
    }
}

void listProducts(int fd, int new_fd)
{

    struct flock lock;
    productReadLock(fd, lock);

    struct product p;
    int i=0;
    while (read(fd, &p, sizeof(struct product)))
    {
        if (p.id != -1)
        {
            //printf("%d\n",i);/////////////////////////////////////////////////
            write(new_fd, &p, sizeof(struct product));
        }
    }

    p.id = -1;
    write(new_fd, &p, sizeof(struct product));
    // unlock(fd, lock);
    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lock);
    printf("Lock unlocked\n");
}

void deleteProduct(int fd, int new_fd, int id, int fd_admin)
{

    struct flock lock;
    productReadLock(fd, lock);
    char response[100];

    struct product p;
    int flg = 0;
    while (read(fd, &p, sizeof(struct product)))
    {
        if (p.id == id)
        {

            unlock(fd, lock);
            productWriteLock(fd, lock);

            p.id = -1;
            strcpy(p.name, "");
            p.price = -1;
            p.qty = -1;

            write(fd, &p, sizeof(struct product));
            write(new_fd, "Delete successful", sizeof("Delete successful"));
            sprintf(response, "Product with product id %d deleted succesfully\n", id);
            write(fd_admin, response, strlen(response));

            unlock(fd, lock);
            flg = 1;
            return;
        }
    }

    if (!flg)
    {
        sprintf(response, "Deleting product with product id %d failed as product does not exist\n", id);
        write(fd_admin, response, strlen(response));
        write(new_fd, "Product id invalid", sizeof("Product id invalid"));
        unlock(fd, lock);
    }
}

void updateProduct(int fd, int new_fd, int ch, int fd_admin)
{
    int id;
    int val = -1;
    struct product p1,p2;
    read(new_fd, &p1, sizeof(struct product));
    id = p1.id;

    char response[100];

    if (ch == 1)
    {
        val = p1.price;
    }
    else
    {
        val = p1.qty;
    }

    struct flock lock;
    printf("Before unlocking\n");
    productReadLock(fd, lock);
    int flg = 0;
    lseek(fd,0,SEEK_SET);
    struct product p;
    while (read(fd, &p, sizeof(struct product)))
    {
        if (p.id == id)
        {
            
            lock.l_type = F_UNLCK;
            fcntl(fd, F_SETLK, &lock);
            printf("Before unlocking\n");
            lseek(fd, (-1) * sizeof(struct product), SEEK_CUR);
            lock.l_type = F_WRLCK;
            lock.l_whence = SEEK_CUR;
            lock.l_start = 0;
            lock.l_len = sizeof(struct product);
            fcntl(fd, F_SETLK, &lock);
            printf("Lock Unlocked\n");
            int old;
            if (ch == 1)
            {
                old = p.price;
                p.price = val;
            }
            else
            {
                old = p.qty;
                p.qty = val;
            }

            write(fd, &p, sizeof(struct product));
            if (ch == 1)
            {
                write(new_fd, "Price modified successfully", sizeof("Price modified successfully"));
                sprintf(response, "Price of product with product id %d modified from %d to %d \n", id, old, val);
                write(fd_admin, response, strlen(response));
            }
            else
            {
                sprintf(response, "Quantity of product with product id %d modified from %d to %d \n", id, old, val);
                write(fd_admin, response, strlen(response));
                write(new_fd, "Quantity modified successfully", sizeof("Quantity modified successfully"));
            }

            lock.l_type = F_UNLCK;
            printf("Before unlocking\n");
            fcntl(fd, F_SETLK, &lock);
            printf("Lock Unlocked\n");
            flg = 1;
            break;
        }
    }

    if (!flg)
    {
        write(new_fd, "Product id invalid", sizeof("Product id invalid"));
        lock.l_type = F_UNLCK;
        fcntl(fd, F_SETLK, &lock);
    }
}

void addCustomer(int fd_cart, int fd_custs, int new_fd)
{
    char buf;
    read(new_fd, &buf, sizeof(char));
    if (buf == 'y')
    {
        int flag=0;
        struct flock lock;
        SetLockCustom(fd_custs, lock);

        int max_id = -1;
        struct index id;
        lseek(fd_custs,0,SEEK_SET);
        while (read(fd_custs, &id, sizeof(struct index)))
        {
            if (id.custid > max_id)
            {
                flag=1;
                max_id = id.custid;
            }
        }

        max_id++;
        if (flag==0)
        {
            max_id=1;
        }
        id.custid = max_id;
        id.offset = lseek(fd_cart, 0, SEEK_END);
        lseek(fd_custs, 0, SEEK_END);
        write(fd_custs, &id, sizeof(struct index));

        struct cart c;
        c.custid = max_id;
        for (int i = 0; i < MAX_PROD; i++)
        {
            c.products[i].id = -1;
            strcpy(c.products[i].name, "");
            c.products[i].qty = -1;
            c.products[i].price = -1;
        }

        write(fd_cart, &c, sizeof(struct cart));
        unlock(fd_custs, lock);
        write(new_fd, &max_id, sizeof(int));
    }
}

void viewCart(int fd_cart, int new_fd, int fd_custs)
{
    int cust_id = -1;
    read(new_fd, &cust_id, sizeof(int));

    int offset = OffSetGetter(cust_id, fd_custs);
    struct cart c;

    if (offset == -1)
    {

        struct cart c;
        c.custid = -1;
        write(new_fd, &c, sizeof(struct cart));
    }
    else
    {
        struct cart c;
        struct flock lock_cart;

        cartOffsetLock(fd_cart, lock_cart, offset, 1);
        read(fd_cart, &c, sizeof(struct cart));
        write(new_fd, &c, sizeof(struct cart));
        unlock(fd_cart, lock_cart);
    }
}

void addProductToCart(int fd, int fd_cart, int fd_custs, int new_fd)
{
    int cust_id = -1;
    read(new_fd, &cust_id, sizeof(int));
    int offset = OffSetGetter(cust_id, fd_custs);

    if (offset == -1)
    {
        return;
    }

    struct flock lock_cart;
    struct product p;
    read(new_fd, &p, sizeof(struct product));
    int i = -1;
    cartOffsetLock(fd_cart, lock_cart, offset, 1);
    struct cart c;
    read(fd_cart, &c, sizeof(struct cart));

    struct flock lock_prod;
    productReadLock(fd, lock_prod);

    int prod_id = p.id;
    int qty = p.qty;

    struct product p1;
    int found = 0;
    while (read(fd, &p1, sizeof(struct product)))
    {
        if (p1.id == p.id)
        {
            if (p1.qty >= p.qty)
            {
                found = 1;
                break;
            }
        }
    }
    lock_prod.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lock_prod);
    printf("Lock Unlocked2\n");
    if (!found)
    {
        write(new_fd, "Please Enter Again.\n", sizeof("Please Enter Again.\n"));
        return;
    }

    int flg = 0;

    int flg1 = 0;
    for (int i = 0; i < MAX_PROD; i++)
    {
        if (c.products[i].id == p.id)
        {
            flg1 = 1;
            break;
        }
    }

    if (flg1)
    {
        write(new_fd, "Product already exists in cart\n", sizeof("Product already exists in cart\n"));
        return;
    }

    for (int i = 0; i < MAX_PROD; i++)
    {
        if (c.products[i].id == -1)
        {
            flg = 1;
            c.products[i].id = p.id;
            c.products[i].qty = p.qty;
            strcpy(c.products[i].name, p1.name);
            c.products[i].price = p1.price;
            break;
        }
        else if (c.products[i].qty <= 0)
        {
            flg = 1;
            c.products[i].id = p.id;
            c.products[i].qty = p.qty;
            strcpy(c.products[i].name, p1.name);
            c.products[i].price = p1.price;
            break;
        }
    }

    if (!flg)
    {
        write(new_fd, "Cart limit reached\n", sizeof("Cart limit reached\n"));
        return;
    }

    write(new_fd, "Item added to cart\n", sizeof("Item added to cart\n"));

    cartOffsetLock(fd_cart, lock_cart, offset, 2);
    write(fd_cart, &c, sizeof(struct cart));
    lock_cart.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lock_cart);
    printf("Lock Unlocked3\n");
}

bool checkQuantity(int fd_items, struct product p)
{
    struct product prod;
    while (read(fd_items, &prod, sizeof(struct product)))
    {
        if (prod.id == p.id)
        {
            struct flock lock_product;
            lock_product.l_whence = SEEK_CUR;
            lock_product.l_len = sizeof(struct product);
            lock_product.l_start = 0;
            lock_product.l_type = F_RDLCK;
            fcntl(fd_items, F_SETLKW, &lock_product);
            if (prod.qty >= p.qty)
            {
                return true;
            }
            else
            {
                return false;
            }
            unlock(fd_items, lock_product);
        }
    }
    return false;
}
void editProductInCart(int fd, int fd_cart, int fd_custs, int new_fd)
{

    int cust_id = -1;
    read(new_fd, &cust_id, sizeof(int));

    int offset = OffSetGetter(cust_id, fd_custs);


    struct flock lock_cart;
    cartOffsetLock(fd_cart, lock_cart, offset, 1);
    struct cart c;
    read(fd_cart, &c, sizeof(struct cart));

    int pid, qty;
    struct product p;
    read(new_fd, &p, sizeof(struct product));

    pid = p.id;
    qty = p.qty;

    int flg = 0;
    int i;
    for (i = 0; i < MAX_PROD; i++)
    {
        if (c.products[i].id == pid)
        {

            struct flock lock_prod;
            productReadLock(fd, lock_prod);

            struct product p1;
            while (read(fd, &p1, sizeof(struct product)))
            {
                if (p1.id == pid && p1.qty > 0)
                {
                    if (p1.qty >= qty)
                    {
                        flg = 1;
                        break;
                    }
                    else
                    {
                        flg = 0;
                        break;
                    }
                }
            }

            unlock(fd, lock_prod);
            break;
        }
    }
    unlock(fd_cart, lock_cart);

    if (!flg)
    {
        write(new_fd, "Product id not in the order or out of stock\n", sizeof("Product id not in the order or out of stock\n"));
        return;
    }

    c.products[i].qty = qty;
    write(new_fd, "Update successful\n", sizeof("Update successful\n"));
    cartOffsetLock(fd_cart, lock_cart, offset, 2);
    write(fd_cart, &c, sizeof(struct cart));
    unlock(fd_cart, lock_cart);
}

void payment(int fd, int fd_cart, int fd_custs, int new_fd)
{
    int cust_id = -1;
    read(new_fd, &cust_id, sizeof(int));

    int offset;
    offset = OffSetGetter(cust_id, fd_custs);

    struct flock lock_cart;
    cartOffsetLock(fd_cart, lock_cart, offset, 1);

    struct cart c;
    read(fd_cart, &c, sizeof(struct cart));
    unlock(fd_cart, lock_cart);
    write(new_fd, &c, sizeof(struct cart));

    int total = 0;

    for (int i = 0; i < MAX_PROD; i++)
    {

        if (c.products[i].id != -1)
        {
            write(new_fd, &c.products[i].qty, sizeof(int));

            struct flock lock_prod;
            productReadLock(fd, lock_prod);
            lseek(fd, 0, SEEK_SET);

            struct product p;
            int flg = 0;
            while (read(fd, &p, sizeof(struct product)))
            {

                if (p.id == c.products[i].id && p.qty > 0)
                {
                    int min;
                    if (c.products[i].qty >= p.qty)
                    {
                        min = p.qty;
                    }
                    else
                    {
                        min = c.products[i].qty;
                    }

                    flg = 1;
                    write(new_fd, &min, sizeof(int));
                    write(new_fd, &p.price, sizeof(int));
                    break;
                }
            }

            unlock(fd, lock_prod);

            if (!flg)
            {
                // product got deleted midway
                int val = 0;
                write(new_fd, &val, sizeof(int));
                write(new_fd, &val, sizeof(int));
            }
        }
    }

    char ch;
    read(new_fd, &ch, sizeof(char));

    for (int i = 0; i < MAX_PROD; i++)
    {

        struct flock lock_prod;
        productReadLock(fd, lock_prod);
        lseek(fd, 0, SEEK_SET);

        struct product p;
        while (read(fd, &p, sizeof(struct product)))
        {

            if (p.id == c.products[i].id)
            {
                int min;
                if (c.products[i].qty >= p.qty)
                {
                    min = p.qty;
                }
                else
                {
                    min = c.products[i].qty;
                }
                unlock(fd, lock_prod);
                productWriteLock(fd, lock_prod);
                p.qty = p.qty - min;

                write(fd, &p, sizeof(struct product));
                unlock(fd, lock_prod);
            }
        }
        unlock(fd, lock_prod);
    }

    cartOffsetLock(fd_cart, lock_cart, offset, 2);

    for (int i = 0; i < MAX_PROD; i++)
    {
        c.products[i].id = -1;
        strcpy(c.products[i].name, "");
        c.products[i].price = -1;
        c.products[i].qty = -1;
    }

    write(fd_cart, &c, sizeof(struct cart));
    write(new_fd, &ch, sizeof(char));
    unlock(fd_cart, lock_cart);

    read(new_fd, &total, sizeof(int));
    read(new_fd, &c, sizeof(struct cart));

    int fd_rec = open("receipt.txt", O_CREAT | O_RDWR, 0777);
    write(fd_rec, "ProductID\tProductName\tQuantity\tPrice\n", strlen("ProductID\tProductName\tQuantity\tPrice\n"));
    char temp[100];
    for (int i = 0; i < MAX_PROD; i++)
    {
        if (c.products[i].id != -1)
        {
            sprintf(temp, "%d\t%s\t%d\t%d\n", c.products[i].id, c.products[i].name, c.products[i].qty, c.products[i].price);
            write(fd_rec, temp, strlen(temp));
        }
    }
    sprintf(temp, "Total - %d\n", total);
    write(fd_rec, temp, strlen(temp));
    close(fd_rec);
}

void generateAdminReceipt(int fd_admin, int fd)
{
    struct flock lock;
    productReadLock(fd, lock);
    write(fd_admin, "Current Inventory:\n", strlen("Current Inventory:\n"));
    write(fd_admin, "ProductID\tProductName\tQuantity\tPrice\n", strlen("ProductID\tProductName\tQuantity\tPrice\n"));

    lseek(fd, 0, SEEK_SET);
    struct product p;
    while (read(fd, &p, sizeof(struct product)))
    {
        if (p.id != -1)
        {
            char temp[100];
            sprintf(temp, "%d\t%s\t%d\t%d\n", p.id, p.name, p.qty, p.price);
            write(fd_admin, temp, strlen(temp));
        }
    }
    unlock(fd, lock);
}
