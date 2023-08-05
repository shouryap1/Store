#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "client.h"

void displayMenuUser()
{
    printf("a->To exit the menu\n");
    printf("b->To see available products\n");
    printf("c->To see your cart\n");
    printf("d->To add products to your cart\n");
    printf("e->To edit an existing product in your cart\n");
    printf("f->To proceed for payment\n");
    printf("g->To register a new customer\n");
    printf("Please enter a choice\n");
}

void displayMenuAdmin()
{
    
    printf("a. To add a product\n");
    printf("b. To delete a product\n");
    printf("c. To update the price of an existing product\n");
    printf("d. To update the quantity of an existing product\n");
    printf("e. To see your inventory\n");
    printf("f. To exit the program\n");
    printf("Please enter a choice\n");
}

void getInventory(int sockfd)
{
    printf("Fetching data\n");
    while (1)
    {
        struct product p;
        read(sockfd, &p, sizeof(struct product));
        if (p.id != -1)
        {
            if (p.qty > 0)
                printf("ProductId : %d Name : %s Quantity : %d Price : %d\n", p.id, p.name, p.qty, p.price);
        }
        else
        {
            break;
        }
    }
}

int calculateTotal(struct cart c)
{
    int total = 0;
    for (int i = 0; i < MAX_PROD; i++)
    {
        if (c.products[i].id != -1)
        {
            total += c.products[i].qty * c.products[i].price;
        }
    }

    return total;
}

void generateReceipt(int total, struct cart c, int sockfd)
{

    write(sockfd, &total, sizeof(int));
    write(sockfd, &c, sizeof(struct cart));
}

// input functions
int prodIdTaker()
{
    int prodId = -1;
    while (1)
    {
        printf("Enter product id\n");
        scanf("%d", &prodId);

        if (prodId < 0)
        {
            printf("Product id can't be negative, please try again\n");
        }
        else
        {
            break;
        }
    }
    return prodId;
}
int priceTaker()
{
    int price = -1;
    while (1)
    {
        printf("Enter price\n");
        scanf("%d", &price);

        if (price < 0)
        {
            printf("Price can't be negative, try again\n");
        }
        else
        {
            break;
        }
    }
    return price;
}
int quantityTaker()
{
    int qty = -1;
    while (1)
    {
        printf("Enter quantity\n");
        scanf("%d", &qty);

        if (qty < 0)
        {
            printf("Quantity can't be negative, try again\n");
        }
        else
        {
            break;
        }
    }
    return qty;
}
// Functions of user.

void GetCartItems(int sockfd, int cusid)
{
    write(sockfd, &cusid, sizeof(int));
    struct cart o;
    read(sockfd, &o, sizeof(struct cart));

    if (o.custid != -1)
    {
        printf("Customer ID %d\n", o.custid);
        for (int i = 0; i < MAX_PROD; i++)
        {
            if (o.products[i].id != -1 && o.products[i].qty > 0)
                printf("ProductId: %d Name: %s Quantity: %d Price: %d\n", o.products[i].id, o.products[i].name, o.products[i].qty, o.products[i].price);
        }
    }
    else
    {
        printf("Wrong customer id provided\n");
    }
}
void AddProductToCart(int sockfd)
{
    char response[80];
    int pid, qty;
    pid = prodIdTaker();

    while (1)
    {
        printf("Enter quantity\n");
        scanf("%d", &qty);

        if (qty <= 0)
        {
            printf("Quantity can't be <= 0, try again\n");
        }
        else
        {
            break;
        }
    }

    struct product p;
    p.id = pid;
    p.qty = qty;

    write(sockfd, &p, sizeof(struct product));
    read(sockfd, response, sizeof(response));
    printf("%s", response);
}
void EditProductInCart(int sockfd)
{
    int pid, qty;
    pid = prodIdTaker();
    qty = quantityTaker();

    struct product p;
    p.id = pid;
    p.qty = qty;

    write(sockfd, &p, sizeof(struct product));

    char response[80];
    read(sockfd, response, sizeof(response));
    printf("%s", response);
}
void ProceedToPayment(int sockfd)
{
    struct cart c;
    read(sockfd, &c, sizeof(struct cart));

    int ordered, instock, price;
    for (int i = 0; i < MAX_PROD; i++)
    {

        if (c.products[i].id != -1)
        {
            read(sockfd, &ordered, sizeof(int));
            read(sockfd, &instock, sizeof(int));
            read(sockfd, &price, sizeof(int));
            printf("Product id- %d\n", c.products[i].id);
            printf("Ordered - %d;  Price - %d\n", ordered, price);
            c.products[i].qty = instock;
            c.products[i].price = price;
        }
    }

    int total = calculateTotal(c);

    printf("Total in your cart\n");
    printf("%d\n", total);
    int payment;

    while (1)
    {
        printf("Please enter the amount to pay\n");
        scanf("%d", &payment);

        if (payment != total)
        {
            printf("Wrong total entered, enter again\n");
        }
        else
        {
            break;
        }
    }

    char ch = 'y';
    printf("Payment recorded, order placed\n");
    write(sockfd, &ch, sizeof(char));
    read(sockfd, &ch, sizeof(char));
    generateReceipt(total, c, sockfd);
}
void AddNewCustomer(int sockfd)
{
    char conf;
    printf("Press y/n if you want to continue\n");
    scanf("%c", &conf);
    scanf("%c", &conf);
    int a = write(sockfd, &conf, sizeof(char));
    if (conf == 'y')
    {
        int id;
        read(sockfd, &id, sizeof(int));
        printf("Your new customer id : %d\n", id);
    }
    else
    {
        printf("Request aborted\n");
    }
}
void AddProduct(int sockfd)
{
    int id, qty, price;
    char name[50];

    printf("Enter product name\n");
    scanf("%s", name);
    read(sockfd, &id, sizeof(int));
    printf("The Product id is %d\n", id);
    qty = quantityTaker();
    price = priceTaker();

    struct product p;
    p.id = id;
    strcpy(p.name, name);
    p.qty = qty;
    p.price = price;

    int n1 = write(sockfd, &p, sizeof(struct product));

    char response[80];
    int n = read(sockfd, response, sizeof(response));
    response[n] = '\0';

    printf("%s", response);
}
void DeleteAProduct(int sockfd)
{
    int id = prodIdTaker();

    write(sockfd, &id, sizeof(int));
    // deleting is equivalent to setting everything as -1

    char response[80];
    read(sockfd, response, sizeof(response));
    printf("%s\n", response);
}
void UpdateProduct(int sockfd, int pri_or_quan)
{
    if (pri_or_quan == 0) // update price
    {
        int id = prodIdTaker();

        int price = priceTaker();
        printf("%d\n", price);
        struct product p;
        p.id = id;
        p.price = price;
        write(sockfd, &p, sizeof(struct product));

        char response[80];
        read(sockfd, response, sizeof(response));
        printf("%s\n", response);
    }
    else if (pri_or_quan == 1)
    {
        int id = prodIdTaker();
        int qty = quantityTaker();
        printf("%d\n", qty);
        struct product p;
        p.id = id;
        p.qty = qty;
        write(sockfd, &p, sizeof(struct product));

        char response[80];
        read(sockfd, response, sizeof(response));
        printf("%s\n", response);
    }
}