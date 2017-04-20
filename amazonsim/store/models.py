from django.db import models
from django.contrib.auth.models import User


class Product(models.Model):
    name = models.TextField()
    description = models.TextField()

    def __str__(self):
        return self.name

class Warehouse(models.Model):
    x_coord = models.IntegerField()
    y_coord = models.IntegerField()

    def __str__(self):
        return "warehouse at {0},{1}".format(self.x_coord, self.y_coord)


class Inventory(models.Model):
    product = models.ForeignKey(Product, on_delete=models.CASCADE)
    count = models.IntegerField()
    warehouse = models.ForeignKey(Warehouse, on_delete=models.CASCADE)

    def __str__(self):
        return "product: {0}, # left: {1}".format(self.name, self.count)

class TrackingNumber(models.Model):
    user = models.ForeignKey(User, on_delete=models.CASCADE)
    fsm_state = models.IntegerField()

    def __str__(self):
        return "tracking number {0}: state {1}".format(self.id, self.fsm_state)

class ShipmentItem(models.Model):
    tracking_number = models.ForeignKey(
        TrackingNumber, on_delete=models.CASCADE)
    product = models.ForeignKey(Product, on_delete=models.CASCADE)
    count = models.IntegerField()

    def __str__(self):
        return "tracking number: {0}\n product: {1} count: {2}".format(
            self.tracking_number, self.product, self.count)


class ShoppingCarts(models.Model):
    user = models.OneToOneField(User, on_delete=models.CASCADE, unique=True)


class CartItem(models.Model):
    shopping_cart = models.ForeignKey(ShoppingCarts, null=True, on_delete=models.CASCADE)
    product = models.ForeignKey(Product, on_delete=models.CASCADE,)
    count = models.IntegerField()

    def __str__(self):
        return "product: {0} count: {1}".format(self.product, self.count)
