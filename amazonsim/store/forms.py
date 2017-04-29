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


class CheckoutForm(forms.Form):
    def __init__(self, *args, **kwargs):
        self.request = kwargs.pop("request")
        if 'x_coord' in self.request.session:
            x_placeholder = self.request.session['x_coord']
        else:
            x_placeholder = None 
        if 'y_coord' in self.request.session:
            y_placeholder = self.request.session['y_coord']
        else:
            y_placeholder = None
        if 'ups_num' in self.request.session:
            ups_num_placeholder = self.request.session['ups_num']
        else:
            ups_num_placeholder = None
        super(CheckoutForm, self).__init__(*args, **kwargs)
        self.fields['x_coord'] = forms.IntegerField(label='x_coord', widget=forms.NumberInput(
            attrs={'class': 'form-control', 'placeholder': x_placeholder}))
        self.fields['y_coord'] = forms.IntegerField(label='y_coord', widget=forms.NumberInput(
            attrs={'class': 'form-control', 'placeholder': y_placeholder}))
        self.fields['ups_num'] = forms.IntegerField(label='ups_num', required=False, widget=forms.NumberInput(
            attrs={'class': 'form-control', 'placeholder': ups_num_placeholder}))


class FilterForm(forms.Form):
    filter_text = forms.CharField(label='Filter Text', max_length=100,
                               widget=forms.TextInput(attrs={'class': 'form-control mr-sm-2',
                                                             'placeholder': 'Search',}))
