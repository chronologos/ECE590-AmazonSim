from django import forms
from store.models import *

class BuyForm(forms.Form):
    qty = forms.IntegerField(label='quantity')

class SignupForm(forms.Form):
    username = forms.CharField(label='Username', max_length=100,
                               widget=forms.TextInput(attrs={'class': 'form-control',
                                                             'placeholder': 'Username', 'autofocus': ''}))
    email = forms.CharField(label='Email', max_length=100,
                            widget=forms.EmailInput(attrs={'class': 'form-control',
                                                           'placeholder': 'Email'}))
    password = forms.CharField(widget=forms.PasswordInput(
        attrs={'class': 'form-control', 'placeholder': 'Password'}))


class LoginForm(forms.Form):
    username = forms.CharField(label='Username', max_length=100,
                               widget=forms.TextInput(attrs={'class': 'form-control',
                                                             'placeholder': 'Username', 'autofocus': ''}))
    password = forms.CharField(widget=forms.PasswordInput(
        attrs={'class': 'form-control', 'placeholder': 'Password'}))
