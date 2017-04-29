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
    truck_id = models.IntegerField() # -1 if no truck
     
    def __str__(self):
        return "warehouse at {0},{1}. truck {2} present.".format(self.x_coord, self.y_coord, self.truck_id)


class Inventory(models.Model):
    product = models.ForeignKey(Product, on_delete=models.CASCADE)
    count = models.IntegerField()
    price = models.IntegerField()
    warehouse = models.ForeignKey(Warehouse, on_delete=models.CASCADE)

    def __str__(self):
        return "product: {0}, # left: {1}".format(self.product.name, self.count)

class TrackingNumber(models.Model):
    # to dispatch, fsm_state 1,2,3,4 must be empty for a particular warehouse and all products must be in state 5.
    # after dispatch, set warehouse truck_id to -1.
    user = models.ForeignKey(User, on_delete=models.CASCADE)
    x_address = models.IntegerField() 
    y_address = models.IntegerField() 
    fsm_state = models.IntegerField() 
    ups_num = models.IntegerField(null=True) 
    # 0 purchased and not arrived
    # 1 asked to pack and asked for truck
    # 2 packed but no truck
    # 3 truck but no pack
    # 4 pack and truck, asking to load
    # 5 waiting to truck to fill with other products
    # 6 dispatched
    # 7 delivered
    truck_id = models.IntegerField() # -1 if no truck
    warehouse = models.ForeignKey(Warehouse, on_delete=models.CASCADE)

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
