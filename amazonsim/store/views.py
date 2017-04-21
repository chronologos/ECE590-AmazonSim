from django.shortcuts import render
from django.http import HttpResponseRedirect, HttpResponse
from store.models import *
from django.urls import reverse
from django.views import generic
from django.contrib.auth.models import User
from django.contrib.auth import authenticate
from django.contrib.auth import logout as llogout
from django.contrib.auth import login as llogin
from django.contrib.auth.mixins import LoginRequiredMixin, UserPassesTestMixin
from django.contrib.auth.decorators import login_required
from .forms import *

from google.protobuf.internal.decoder import _DecodeVarint32
from google.protobuf.internal.encoder import _EncodeVarint
import internalcom_pb2
import socket
import struct

CPP_HOST = "localhost"
CPP_PORT = 12345
SOCKET_TIMEOUT = 0.5
# Create your views here.


class InventoryView(generic.ListView):
    template_name = 'store/inventory.html'
    context_object_name = 'inventory_items'

    def get_queryset(self):
        """Return the last five published questions."""
        return Inventory.objects.all()


def buy(request, product_id):
    if request.method == 'POST':
        form = BuyForm(request.POST)
        if form.is_valid():
            qty = form.cleaned_data['qty']
            print(
                "purchase for qty={0} of product_id={1} received".format(
                    product_id, qty)
            )
            c = getcart(request)
            p = Product.objects.get(id=product_id)
            if (CartItem.objects.filter(shopping_cart=c).filter(product=p).exists()):
                item = CartItem.objects.filter(shopping_cart=c).get(product=p)
                item.count += qty
            else:
                item = CartItem(product=p, count=qty, shopping_cart=c)
            item.save()
            return HttpResponseRedirect(reverse('products'))
    else:
        form = BuyForm()
        context = {'product_id': product_id, 'form': form}
    return render(request, 'store/buy.html', context)


def cart(request):
    # TODO return list or queryset to render?
    cart = getcart(request)
    cart_items = CartItem.objects.filter(shopping_cart_id=cart.id).all()
    context = {'cart_items': cart_items}
    print(cart_items)
    return render(request, 'store/cart.html', context)


def checkout(request):
    cart = getcart(request)
    cart_items = CartItem.objects.filter(shopping_cart_id=cart.id).all()
    tracking_number = TrackingNumber(fsm_state=0, user=request.user)
    tracking_number.save()

    for item in cart_items:
        product = item.product
        count = item.count
        inventory_item = Inventory.objects.get(product=product)
        inventory_item.count -= count
        inventory_item.save()
        shipment_item = ShipmentItem(tracking_number=tracking_number, product=product, count=count)
        shipment_item.save()

    # send tracking_number to CPP server
    order = internalcom_pb2.Order()
    order.shipid = tracking_number.id
    print(order.shipid)
    order_bytes = order.SerializeToString()
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.settimeout(SOCKET_TIMEOUT)
        bytes_size = len(order_bytes)
        print("size is {0}".format(bytes_size))
        s.connect((CPP_HOST, CPP_PORT))
        s.sendall(struct.pack("!q", bytes_size))
        s.sendall(order_bytes)
        # minus from Inventory
        # make shipment item
        # return tracking number

    CartItem.objects.filter(shopping_cart_id=cart.id).delete()
    context = {'tracking_number': tracking_number.id}
    return render(request, 'store/checkout.html', context)

def orders(request):
    orders = TrackingNumber.objects.filter(user=request.user).all()
    ordermap = {}
    for order in orders:
        items = ShipmentItem.objects.filter(tracking_number = order.id).all()
        ordermap[order.id] = items
    context = {"orders" : ordermap}
    print(ordermap)
    return render(request, 'store/orders.html', context)


def get_signup(request):
    if request.method == 'POST':
        form = SignupForm(request.POST)
        if form.is_valid():
            username = form.cleaned_data['username']
            email = form.cleaned_data['email']
            password = form.cleaned_data['password']
            user = User.objects.create_user(username=username,
                                            email=email, password=password)
            user.save()
            return HttpResponseRedirect(reverse('login'))
    else:
        form = SignupForm()

    return render(request, 'store/signup.html', {'form': form})


def login(request):
    if request.method == 'POST':
        form = LoginForm(request.POST)
        if form.is_valid():
            username = form.cleaned_data['username']
            password = form.cleaned_data['password']
            user = authenticate(username=username, password=password)
            if user is not None:
                llogin(request, user)
                return HttpResponseRedirect(reverse('products'))

    else:
        form = LoginForm()

    return render(request, 'store/login.html', {'form': form})

def logout(request):
    logout(request)
    return HttpResponseRedirect(reverse('login'))

def getcart(request):
    """ Helper that returns a user's cart, creating if it doesn't exist."""
    if not (ShoppingCarts.objects.filter(user=request.user).exists()):
        cart = ShoppingCarts(user=request.user)
        cart.save()
    else:
        cart = ShoppingCarts.objects.get(user=request.user)
    return cart
