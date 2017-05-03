import random
from django.shortcuts import render
from django.http import HttpResponseRedirect, HttpResponse, JsonResponse
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
from google.protobuf.internal.encoder import _EncodeVarint as varintEncoder
import internalcom_pb2
import amazon_pb2
import socket
import struct

PYTHON_PORT = "8000"

UPS_HOST = "localhost"
UPS_PORT = "8000"

CPP_HOST = "localhost"
# CPP_PORT = 12345
CPP_PORT = 23456
SOCKET_TIMEOUT = 5
SIM_HOST = "10.236.48.28"
#SIM_HOST = "10.190.87.71"
#SIM_HOST = "localhost"
SIM_PORT = 23456
WORLD = 1001
N_WAREHOUSES = 2

# ad hoc data structure containing map of product_name:(count,price,description)
PRODUCTS = {"papaya":(10000,10,"Delicious fruit!"), "tomato":(10000,20,"Good for soup."), "aop":(10000,5,"essential reading?")}
# Create your views here.

def init(request):
    # populate database for both sim and c++/py
    
    # this part is to make the api idempotent (at most once)
    if Product.objects.all().exists() or Warehouse.objects.all().exists():
        return render(request, 'store/cart.html',
                      {"info": "Already inited, not doing anything."})
    sim_connect = amazon_pb2.AConnect()
    sim_connect.worldid = WORLD
    sim_connect_bytes = sim_connect.SerializeToString()
    
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.settimeout(SOCKET_TIMEOUT)
            bytes_size = len(sim_connect_bytes)
            print("size is {0}".format(bytes_size))
            s.connect((SIM_HOST, SIM_PORT))
            varintEncoder(s.send, bytes_size)
            s.sendall(sim_connect_bytes)
            a = s.recv(1024) # handle this

            warehouses = []
            for w in range(N_WAREHOUSES):
               w_local = Warehouse(x_coord=0,y_coord=0,truck_id=-1)
               w_local.save() 
               warehouses.append(w_local)

            command_pb = amazon_pb2.ACommands()
            command_pb.simspeed = 40000000
            command_pb.disconnect = True
            prod_map = {}
            for name,tup in PRODUCTS.items():
                count, price, description = tup
                p_local = Product(name=name, description=description, rating=0, num_ratings=0)
                p_local.save()
                prod_map[name] = p_local
            for w in warehouses:
                purchase_pb = command_pb.buy.add()
                # purchase_pb = amazon_pb2.APurchaseMore()
                purchase_pb.whnum = w.id
                for name,tup in PRODUCTS.items():
                    count, price, description = tup
                    i_local = Inventory(product=prod_map[name], count=count, price=price, warehouse=w)
                    i_local.save()
                    thing = purchase_pb.things.add()
                    thing.id = prod_map[name].id
                    thing.description = description
                    thing.count = count
            command_pb_bytes = command_pb.SerializeToString()
            print(command_pb_bytes) 
            bytes_size = len(command_pb_bytes)
            print("size is {0}".format(bytes_size))
            varintEncoder(s.send, bytes_size)
            s.sendall(command_pb_bytes)
            a = s.recv(1024) # handle this
            print(a)
            s.close()

    except socket.timeout:
        return render(request, 'store/cart.html',
                      {"message": "backend error: socket timed out/conn refused."})
    except ConnectionRefusedError:
        return render(request, 'store/cart.html',
                      {"message": "backend error: socket timed out/conn refused."})
    print("done")
    return render(request, 'store/cart.html',
                  {"info": "init succeeded."})

def rate(request):
    if request.POST:
        print("post?") 
        print(str(request.POST))
        print(request.POST['productid'])
        product_id = request.POST['productid']
        rating = request.POST['rating']
        product = Product.objects.get(id=product_id)
        try: 
            rating = float(rating)
            if rating > 5 or rating < 0:
                return HttpResponseRedirect(reverse('products'))
            product.rating = (product.rating*product.num_ratings + rating)/(product.num_ratings+1)
            product.num_ratings += 1
            product.save()
            return HttpResponseRedirect(reverse('products'))
        except TypeError as t:
            return HttpResponseRedirect(reverse('products'))
    if not request.GET:
        print("gg") 
        return JsonResponse({"rating": "error"})
    else:
        rating = request.GET['rating']
        print(rating)
        return JsonResponse({"rating": rating})

class InventoryView(generic.ListView):
    template_name = 'store/inventory.html'
    context_object_name = 'inventory_items'

    def get_queryset(self):
        """Return the last five published questions."""
        if not self.request.GET:
            return Inventory.objects.all()
        else:
            print(self.request.GET)
            if 'filter_text' in self.request.GET:
                filter_text = self.request.GET['filter_text']
                self.request.session['saved_search'] = filter_text
            else:
                filter_text = ""
            return Inventory.objects.filter(
                product__name__contains=filter_text).all().union(
                Inventory.objects.filter(
                    product__description__contains=filter_text).all())

    def get_context_data(self, **kwargs):
            # Call the base implementation first to get a context
        shipitems_purchased_before = ShipmentItem.objects.filter(
            tracking_number__user=self.request.user).all()
        products_purchased_before = Product.objects.filter(
            shipmentitem__in=shipitems_purchased_before).distinct()
        context = super(InventoryView, self).get_context_data(**kwargs)
        context['purchased_before'] = set(products_purchased_before)
        return context


@login_required(login_url='/')
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
        p = Product.objects.get(id=product_id)
        context = {'product_id': product_id, 'form': form, 'product': p}
    return render(request, 'store/buy.html', context)


def remove(request, product_id):
    cart = getcart(request)
    CartItem.objects.filter(
        shopping_cart_id=cart.id).filter(
        product=product_id).delete()
    print(
        CartItem.objects.filter(
            shopping_cart_id=cart.id).filter(
            product=product_id).all())
    return render(request, 'store/cart.html', {"info": "removed items!"})


@login_required(login_url='/')
def cart(request):
    # TODO return list or queryset to render?
    cart = getcart(request)
    cart_items = CartItem.objects.filter(shopping_cart_id=cart.id).all()
    context = {'cart_items': cart_items}
    print(cart_items)
    return render(request, 'store/cart.html', context)


@login_required(login_url='/')
def checkout(request):
    cart = getcart(request)
    cart_items = CartItem.objects.filter(shopping_cart_id=cart.id).all()
    # TODO/NOTE this only works if all warehouses hold same inventory
    warehouse = random.choice(Warehouse.objects.all())
    print("chose warehouse {0}".format(warehouse))
    total_price = 0
    if request.method == 'POST':
        form = CheckoutForm(request.POST, request=request)
        if form.is_valid():
            x_coord = form.cleaned_data['x_coord']
            y_coord = form.cleaned_data['y_coord']

            if 'ups_num' in form.cleaned_data: 
                ups_num = form.cleaned_data['ups_num']
            else:
                ups_num = 0

            if 'gift_code' in form.cleaned_data:
                gift_code = form.cleaned_data['gift_code']
                if Giftcard.objects.filter(gift_card_code=gift_code).exists():
                    giftcard = Giftcard.objects.get(gift_card_code=gift_code)
                    if giftcard.used:
                        return render(request, 'store/checkout.html',
                              {'message': 'code used'})
                    else:
                        total_price -= giftcard.value
                        giftcard.used = True
                        giftcard.save()
                else:
                    return render(request, 'store/checkout.html',
                          {'message': 'code invalid'})

            # save address and ups num for convenience, used in CheckoutForm
            # placeholder
            request.session['x_coord'] = x_coord
            request.session['y_coord'] = y_coord
            request.session['ups_num'] = ups_num
        else:
            return render(request, 'store/checkout.html',
                          {'message': 'form invalid'})

        tracking_number = TrackingNumber(
            fsm_state=1,
            user=request.user,
            x_address=x_coord,
            y_address=y_coord,
            truck_id=-1,
            warehouse_id=warehouse.id,
            ups_num=ups_num)
        tracking_number.save()

        for item in cart_items:
            if not Inventory.objects.filter(
                    product=item.product).exists():
                print("ok")
                print(Inventory.objects.filter(
                    product=item.product))
                return render(request, 'store/checkout.html',
                              {"message": "product out of stock"})

            inventory_item = Inventory.objects.filter(
                warehouse=warehouse).get(
                product=item.product)
            total_price += (inventory_item.price * item.count)
            if inventory_item.count < item.count:
                return render(request, 'store/checkout.html',
                              {"message": "not enough stock to fulfill request"})
            product = item.product
            count = item.count
            inventory_item.count -= count
            inventory_item.save()
            shipment_item = ShipmentItem(
                tracking_number=tracking_number,
                description=product.description,
                product=product,
                count=count)
            shipment_item.save()

        # send tracking_number to CPP server
        order = internalcom_pb2.Order()
        order.shipid = tracking_number.id
        order.whid = warehouse.id
        order.delX = x_coord
        order.delY = y_coord
        # print(order.shipid)
        order_bytes = order.SerializeToString()

        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.settimeout(SOCKET_TIMEOUT)
                bytes_size = len(order_bytes)
                print("size is {0}".format(bytes_size))
                s.connect((CPP_HOST, CPP_PORT))
                varintEncoder(s.send, bytes_size)
                # s.sendall(struct.pack("!q", bytes_size))
                s.sendall(order_bytes)
                # minus from Inventory
                # make shipment item
                # return tracking number
        except socket.timeout:
            return render(request, 'store/checkout.html',
                          {"message": "backend error: socket timed out."})

        except ConnectionRefusedError:
            return render(request, 'store/cart.html',
                          {"message": "backend error: socket timed out/conn refused."})
        
        CartItem.objects.filter(shopping_cart_id=cart.id).delete()
        context = {
            'info': 'checked out! your tracking number is {0}'.format(
                tracking_number.id),
            'price': total_price}
        return render(request, 'store/checkout.html', context)
    else:
        form = CheckoutForm(request=request)
        for item in cart_items:
            if not Inventory.objects.filter(
                    product=item.product).exists():
                print("ok")
                print(Inventory.objects.filter(
                    product=item.product))
                return render(request, 'store/checkout.html',
                              {"message": "product(s) out of stock"})

            inventory_item = Inventory.objects.filter(
                warehouse=warehouse).get(
                product=item.product)
            total_price += (inventory_item.price * item.count)
        return render(request, 'store/checkout.html',
                      {'form': form, 'price': total_price})


@login_required(login_url='/')
def orders(request):
    orders = TrackingNumber.objects.filter(user=request.user).all()
    ordermap = {}
    for order in orders:
        items = ShipmentItem.objects.filter(tracking_number=order.id).all()
        ordermap[order.id] = items
    context = {"orders": ordermap}
    print(ordermap)
    return render(request, 'store/orders.html', context)


def ship_id_endpoint(request, ship_id):
    if TrackingNumber.objects.filter(id=ship_id).exists():
        tracking_number = TrackingNumber.objects.get(id=ship_id)
        ship_items = ShipmentItem.objects.filter(tracking_number=ship_id)
        ups_num = tracking_number.ups_num
        context = {
            'title': "Shipment # {0}".format(ship_id),
            "ship_items": ship_items,
            'message': "",
            'ups_num': ups_num,
            'status_url': "{0}:{1}/{2}".format(
                UPS_HOST,
                UPS_PORT,
                ship_id)}
        return render(request, 'store/ship_id_endpoint.html', context)
    else:
        context = {'title': "Error", 'message': "No such shipping id"}
        return render(request, 'store/ship_id_endpoint.html', context)


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
    llogout(request)
    return render(request, 'store/loggedout.html', {})


def getcart(request):
    """ Helper that returns a user's cart, creating if it doesn't exist."""
    if not (ShoppingCarts.objects.filter(user=request.user).exists()):
        cart = ShoppingCarts(user=request.user)
        cart.save()
    else:
        cart = ShoppingCarts.objects.get(user=request.user)
    return cart
