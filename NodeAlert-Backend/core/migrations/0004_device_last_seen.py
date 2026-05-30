# Migration to add last_seen to Device model
# Generated manually
from django.db import migrations, models


class Migration(migrations.Migration):

    dependencies = [
        ('core', '0003_userprofile'),
    ]

    operations = [
        migrations.AddField(
            model_name='device',
            name='last_seen',
            field=models.DateTimeField(blank=True, null=True),
        ),
    ]
