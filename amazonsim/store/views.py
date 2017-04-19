from django.shortcuts import render
from django.http import HttpResponseRedirect
from store.models import Inventory, Warehouse
from django.urls import reverse
from django.views import generic

# Create your views here.

class InventoryView(generic.ListView):
    template_name = 'store/inventory.html'
    context_object_name = 'inventory_items'

    def get_queryset(self):
        """Return the last five published questions."""
        return Inventory.objects.all()
