# -*- coding: utf-8 -*-
# Generated by Django 1.11 on 2017-05-03 02:56
from __future__ import unicode_literals

from django.db import migrations, models


class Migration(migrations.Migration):

    dependencies = [
        ('store', '0017_auto_20170503_0236'),
    ]

    operations = [
        migrations.CreateModel(
            name='Giftcard',
            fields=[
                ('id', models.AutoField(auto_created=True, primary_key=True, serialize=False, verbose_name='ID')),
                ('gift_card_code', models.TextField()),
                ('value', models.FloatField()),
                ('used', models.BooleanField()),
            ],
        ),
    ]
