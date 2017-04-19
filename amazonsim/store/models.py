from django.db import models

class Inventory(models.Model):
    name = models.TextField()
    description = models.TextField()
    stock = models.IntegerField()
    warehouse = models.ForeignKey('Warehouse', on_delete=models.CASCADE)

    def __str__(self):
        return "product: {0}, # left: {1}".format(self.name, self.stock)

class FSM(models.Model):
    shipping_id = models.IntegerField()
    fsm_state = models.IntegerField()

    def __str__(self):
        return "id {0}: state {1}".format(self.shipping_id, self.fsm_state)

class Warehouse(models.Model):
    x_coord = models.IntegerField()
    y_coord = models.IntegerField()

    def __str__(self):
        return "warehouse at {0},{1}".format(self.x_coord, self.y_coord)
