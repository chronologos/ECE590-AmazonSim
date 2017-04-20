"""amazonsim URL Configuration

The `urlpatterns` list routes URLs to views. For more information please see:
    https://docs.djangoproject.com/en/1.11/topics/http/urls/
Examples:
Function views
    1. Add an import:  from my_app import views
    2. Add a URL to urlpatterns:  url(r'^$', views.home, name='home')
Class-based views
    1. Add an import:  from other_app.views import Home
    2. Add a URL to urlpatterns:  url(r'^$', Home.as_view(), name='home')
Including another URLconf
    1. Import the include() function: from django.conf.urls import url, include
    2. Add a URL to urlpatterns:  url(r'^blog/', include('blog.urls'))
"""
from django.conf.urls import url
from django.contrib import admin
from store import views

urlpatterns = [
    url(r'^admin/', admin.site.urls),
    url(r'^buy/(?P<product_id>[0-9]+)/$', views.buy, name='buy'),
    url(r'^cart/$', views.cart, name='cart'),
    url(r'^checkout/$', views.checkout, name='checkout'),
    url(r'^logout/$', views.logout, name='logout'),
    url(r'^orders/$', views.orders, name='orders'),
    url(r'^products/', views.InventoryView.as_view(), name='products'),
    url(r'^signup/$', views.get_signup, name='signup'),
    url(r'^$', views.login, name='login'),
]
