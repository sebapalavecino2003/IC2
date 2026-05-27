from django.core.management.base import BaseCommand
from django.contrib.auth.models import Group, Permission
from django.contrib.contenttypes.models import ContentType
from core.models import Device, Event, Reading


class Command(BaseCommand):
    help = 'Create default groups (admin, analyst) with model permissions'

    def handle(self, *args, **options):
        ct_map = ContentType.objects.get_for_models(Device, Event, Reading)

        admin_group, _ = Group.objects.get_or_create(name='admin')
        admin_codenames = [
            'add_device', 'change_device', 'delete_device', 'view_device',
            'add_event', 'change_event', 'delete_event', 'view_event',
            'add_reading', 'view_reading',
        ]
        admin_perms = Permission.objects.filter(
            content_type__in=ct_map.values(),
            codename__in=admin_codenames,
        )
        admin_group.permissions.set(admin_perms)
        self.stdout.write(
            self.style.SUCCESS(f'Group "admin" created with {admin_perms.count()} permissions')
        )

        analyst_group, _ = Group.objects.get_or_create(name='analyst')
        analyst_codenames = [
            'view_device', 'view_reading', 'view_event', 'change_event',
        ]
        analyst_perms = Permission.objects.filter(
            content_type__in=ct_map.values(),
            codename__in=analyst_codenames,
        )
        analyst_group.permissions.set(analyst_perms)
        self.stdout.write(
            self.style.SUCCESS(f'Group "analyst" created with {analyst_perms.count()} permissions')
        )
