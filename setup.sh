#!/bin/bash
# =============================================================================
# NodeAlert IoT — Single-command deployment script
# =============================================================================
# This script automates the complete deployment of the NodeAlert IoT backend:
#   1. Architecture detection (arm64 / amd64)
#   2. Interactive configuration prompts
#   3. .env file generation
#   4. Mosquitto password file generation
#   5. Docker Compose service startup
#   6. Django database migrations
#   7. Django superuser creation
#   8. Firmware configuration header generation
#
# Usage: ./setup.sh [OPTIONS]
#   --help, -h    Show this help message and exit
# =============================================================================

set -euo pipefail

# ---------------------------------------------------------------------------
# A. Help / Usage
# ---------------------------------------------------------------------------
show_help() {
  cat <<EOF
Usage: ./setup.sh [OPTIONS]

Deploy NodeAlert IoT backend stack (Mosquitto + MySQL + Django).
Generates firmware configuration headers for ESP32 compilation.

Options:
  --help, -h    Show this help message and exit

Environment:
  All configuration is collected interactively.
  Generated files: .env, mosquitto_passwd, firmware config headers.
EOF
  exit 0
}

if [ "${1:-}" = "--help" ] || [ "${1:-}" = "-h" ]; then
  show_help
fi

# ---------------------------------------------------------------------------
# B. Architecture Detection (D-21)
# ---------------------------------------------------------------------------
ARCH=$(uname -m)
ARCH_LABEL=""
case "$ARCH" in
  aarch64|arm64)
    ARCH_LABEL="arm64 (Raspberry Pi / ARM)"
    ;;
  x86_64|amd64)
    ARCH_LABEL="amd64 (x86_64 development machine)"
    ;;
  *)
    ARCH_LABEL="$arch (unknown — assuming amd64)"
    ;;
esac

echo "============================================="
echo " NodeAlert IoT — Deployment Setup"
echo "============================================="
echo ""
echo "Detected architecture: $ARCH_LABEL"
echo "Docker images support multi-arch automatically."
echo ""

# ---------------------------------------------------------------------------
# C. Interactive Configuration Prompts
# ---------------------------------------------------------------------------
echo "============================================="
echo " Configuration"
echo "============================================="
echo "Enter values or press Enter to accept defaults."
echo ""

# WiFi (for firmware config generation)
read -r -p "WiFi SSID [NodeAlert-IoT]: " WIFI_SSID
WIFI_SSID="${WIFI_SSID:-NodeAlert-IoT}"

read -r -p "WiFi Password: " WIFI_PASS
WIFI_PASS="${WIFI_PASS:-}"

# MQTT Broker
read -r -p "MQTT Broker User [nodealert_gateway]: " MQTT_BROKER_USER
MQTT_BROKER_USER="${MQTT_BROKER_USER:-nodealert_gateway}"

read -r -p "MQTT Broker Password: " MQTT_BROKER_PASSWORD
MQTT_BROKER_PASSWORD="${MQTT_BROKER_PASSWORD:-}"

while [ -z "$MQTT_BROKER_PASSWORD" ]; do
  read -r -p "MQTT Broker Password (required): " MQTT_BROKER_PASSWORD
done

# Firmware MQTT Broker IP (the Raspberry Pi's LAN IP)
read -r -p "Firmware MQTT Broker IP/Hostname (Raspberry Pi LAN IP): " MQTT_BROKER_IP
MQTT_BROKER_IP="${MQTT_BROKER_IP:-}"

while [ -z "$MQTT_BROKER_IP" ]; do
  read -r -p "Firmware MQTT Broker IP/Hostname (required — e.g. 192.168.1.100): " MQTT_BROKER_IP
done

# MySQL
read -r -p "MySQL Root Password: " MYSQL_ROOT_PASSWORD
MYSQL_ROOT_PASSWORD="${MYSQL_ROOT_PASSWORD:-}"

while [ -z "$MYSQL_ROOT_PASSWORD" ]; do
  read -r -p "MySQL Root Password (required): " MYSQL_ROOT_PASSWORD
done

read -r -p "MySQL Database Name [nodealert]: " MYSQL_DATABASE
MYSQL_DATABASE="${MYSQL_DATABASE:-nodealert}"

read -r -p "MySQL User [nodealert]: " MYSQL_USER
MYSQL_USER="${MYSQL_USER:-nodealert}"

read -r -p "MySQL Password: " MYSQL_PASSWORD
MYSQL_PASSWORD="${MYSQL_PASSWORD:-}"

while [ -z "$MYSQL_PASSWORD" ]; do
  read -r -p "MySQL Password (required): " MYSQL_PASSWORD
done

# Django
DJANGO_SECRET_KEY=$(python3 -c "import secrets; print(secrets.token_urlsafe(50))" 2>/dev/null || echo "")
if [ -z "$DJANGO_SECRET_KEY" ]; then
  # Fallback if python3 not available
  DJANGO_SECRET_KEY="django-insecure-$(date +%s)-$$-$(hostname)"
fi
read -r -p "Django Secret Key (auto-generated: ${DJANGO_SECRET_KEY:0:20}...): " DJANGO_SECRET_KEY_INPUT
DJANGO_SECRET_KEY="${DJANGO_SECRET_KEY_INPUT:-$DJANGO_SECRET_KEY}"

read -r -p "Django Debug (False for production) [False]: " DJANGO_DEBUG
DJANGO_DEBUG="${DJANGO_DEBUG:-False}"

read -r -p "Django Superuser Username [admin]: " DJANGO_SUPERUSER_USERNAME
DJANGO_SUPERUSER_USERNAME="${DJANGO_SUPERUSER_USERNAME:-admin}"

read -r -p "Django Superuser Email [admin@nodealert.local]: " DJANGO_SUPERUSER_EMAIL
DJANGO_SUPERUSER_EMAIL="${DJANGO_SUPERUSER_EMAIL:-admin@nodealert.local}"

read -r -p "Django Superuser Password: " DJANGO_SUPERUSER_PASSWORD
DJANGO_SUPERUSER_PASSWORD="${DJANGO_SUPERUSER_PASSWORD:-}"

while [ -z "$DJANGO_SUPERUSER_PASSWORD" ]; do
  read -r -p "Django Superuser Password (required): " DJANGO_SUPERUSER_PASSWORD
done

echo ""
echo "Configuration complete."
echo ""

# ---------------------------------------------------------------------------
# D. Generate .env file
# ---------------------------------------------------------------------------
echo "Generating .env file..."

cat > .env <<ENVEOF
# =============================================================================
# NodeAlert IoT — Environment Configuration
# Auto-generated by setup.sh — do not edit manually
# =============================================================================

# MQTT Broker
MQTT_BROKER_USER=${MQTT_BROKER_USER}
MQTT_BROKER_PASSWORD=${MQTT_BROKER_PASSWORD}

# MQTT Broker — Firmware connection
MQTT_BROKER_IP=${MQTT_BROKER_IP}

# MySQL Database
MYSQL_ROOT_PASSWORD=${MYSQL_ROOT_PASSWORD}
MYSQL_DATABASE=${MYSQL_DATABASE}
MYSQL_USER=${MYSQL_USER}
MYSQL_PASSWORD=${MYSQL_PASSWORD}

# Django
DJANGO_SECRET_KEY=${DJANGO_SECRET_KEY}
DJANGO_DEBUG=${DJANGO_DEBUG}
DJANGO_SUPERUSER_USERNAME=${DJANGO_SUPERUSER_USERNAME}
DJANGO_SUPERUSER_EMAIL=${DJANGO_SUPERUSER_EMAIL}
DJANGO_SUPERUSER_PASSWORD=${DJANGO_SUPERUSER_PASSWORD}
ENVEOF

chmod 600 .env
echo "  .env created (permissions: 600)"
echo ""

# ---------------------------------------------------------------------------
# E. Generate mosquitto_passwd
# ---------------------------------------------------------------------------
echo "Generating Mosquitto password file..."

if command -v mosquitto_passwd &>/dev/null; then
  MOSQUITTO_PASSWD_DIR="mosquitto/config"
  MOSQUITTO_PASSWD_FILE="${MOSQUITTO_PASSWD_DIR}/mosquitto_passwd"

  mkdir -p "$MOSQUITTO_PASSWD_DIR"
  # Remove existing file if present (mosquitto_passwd -b appends)
  rm -f "$MOSQUITTO_PASSWD_FILE"

  if mosquitto_passwd -b "$MOSQUITTO_PASSWD_FILE" "$MQTT_BROKER_USER" "$MQTT_BROKER_PASSWORD"; then
    echo "  mosquitto_passwd generated at ${MOSQUITTO_PASSWD_FILE}"
  else
    echo "  ERROR: mosquitto_passwd command failed."
    exit 1
  fi
else
  echo "  mosquitto_passwd binary not found. Attempting to install..."
  if command -v apt-get &>/dev/null; then
    apt-get update -qq && apt-get install -y -qq mosquitto-clients
  elif command -v brew &>/dev/null; then
    brew install mosquitto
  else
    echo "  ERROR: Cannot install mosquitto-clients automatically."
    echo "  Please install manually:"
    echo "    Debian/Ubuntu: apt-get install -y mosquitto-clients"
    echo "    macOS:         brew install mosquitto"
    exit 1
  fi

  # Retry after install
  MOSQUITTO_PASSWD_DIR="mosquitto/config"
  MOSQUITTO_PASSWD_FILE="${MOSQUITTO_PASSWD_DIR}/mosquitto_passwd"
  mkdir -p "$MOSQUITTO_PASSWD_DIR"
  rm -f "$MOSQUITTO_PASSWD_FILE"

  if mosquitto_passwd -b "$MOSQUITTO_PASSWD_FILE" "$MQTT_BROKER_USER" "$MQTT_BROKER_PASSWORD"; then
    echo "  mosquitto_passwd generated at ${MOSQUITTO_PASSWD_FILE}"
  else
    echo "  ERROR: mosquitto_passwd command failed after install."
    exit 1
  fi
fi

# Verify password file is non-empty
if [ ! -s "$MOSQUITTO_PASSWD_FILE" ]; then
  echo "  ERROR: mosquitto_passwd file is empty."
  exit 1
fi
echo "  Password file size: $(wc -c < "$MOSQUITTO_PASSWD_FILE") bytes"
echo ""

# ---------------------------------------------------------------------------
# F. Start Docker Compose
# ---------------------------------------------------------------------------
echo "Starting Docker Compose services..."
if ! command -v docker &>/dev/null; then
  echo "  ERROR: Docker is not installed. Please install Docker first."
  echo "  See: https://docs.docker.com/engine/install/"
  exit 1
fi

if ! docker compose version &>/dev/null; then
  echo "  ERROR: Docker Compose V2 is not available."
  echo "  Ensure 'docker compose' (V2) is installed."
  exit 1
fi

echo "  Running: docker compose up -d"
docker compose up -d
COMPOSE_EXIT=$?
if [ "$COMPOSE_EXIT" -ne 0 ]; then
  echo "  ERROR: docker compose up failed with exit code $COMPOSE_EXIT."
  exit 1
fi

echo ""
echo "Waiting for MySQL to be ready..."
MYSQL_READY=false
for i in $(seq 1 30); do
  if docker compose exec mysql mysqladmin ping -u root -p"${MYSQL_ROOT_PASSWORD}" --silent 2>/dev/null; then
    MYSQL_READY=true
    echo "  MySQL is ready!"
    break
  fi
  echo "  Waiting... ($i/30)"
  sleep 2
done

if [ "$MYSQL_READY" = false ]; then
  echo "  ERROR: MySQL did not become ready within 60 seconds."
  echo "  Check container logs: docker compose logs mysql"
  exit 1
fi
echo ""

# ---------------------------------------------------------------------------
# G. Run Django migrations
# ---------------------------------------------------------------------------
echo "Running Django database migrations..."
if docker compose exec django python manage.py migrate --noinput; then
  echo "  Migrations applied successfully."
else
  echo "  ERROR: Django migrations failed."
  exit 1
fi
echo ""

# ---------------------------------------------------------------------------
# H. Create Django superuser
# ---------------------------------------------------------------------------
echo "Creating Django superuser..."
if docker compose exec -e DJANGO_SUPERUSER_PASSWORD="$DJANGO_SUPERUSER_PASSWORD" \
  django python manage.py createsuperuser \
  --username "$DJANGO_SUPERUSER_USERNAME" \
  --email "$DJANGO_SUPERUSER_EMAIL" \
  --noinput 2>/dev/null; then
  echo "  Superuser '${DJANGO_SUPERUSER_USERNAME}' created."
else
  echo "  Superuser may already exist (non-fatal — continuing)."
fi
echo ""

# ---------------------------------------------------------------------------
# I. Generate firmware configuration header (D-22)
# ---------------------------------------------------------------------------
echo "Generating firmware configuration header..."

FIRMWARE_CONFIG_DIR="NodeAlert-Firmware/src/config"
FIRMWARE_CONFIG_FILE="${FIRMWARE_CONFIG_DIR}/user_config.h"
PLATFORMIO_INI="NodeAlert-Firmware/platformio.ini"

mkdir -p "$FIRMWARE_CONFIG_DIR"

cat > "$FIRMWARE_CONFIG_FILE" <<CONFIGEOF
/**
 * @file user_config.h
 * @brief Deploy-time firmware configuration — auto-generated by setup.sh
 *
 * WARNING: This file is auto-generated. Do not edit manually.
 * Re-run setup.sh to update.
 *
 * This file is included before all source files via the -include compiler
 * flag in platformio.ini, overriding the #ifndef defaults in wifi_config.h
 * and mqtt_config.h.
 */

#pragma once

/* ------------------------------------------------------------------------ */
/* WiFi Credentials (overrides wifi_config.h defaults)                      */
/* ------------------------------------------------------------------------ */
#define WIFI_SSID               "${WIFI_SSID}"
#define WIFI_PASS               "${WIFI_PASS}"

/* ------------------------------------------------------------------------ */
/* MQTT Broker (overrides mqtt_config.h defaults)                           */
/* ------------------------------------------------------------------------ */
#define MQTT_BROKER_URI         "mqtt://${MQTT_BROKER_IP}"
#define MQTT_PORT               1883
CONFIGEOF

echo "  Firmware config generated: ${FIRMWARE_CONFIG_FILE}"

# Add -include compiler flag to platformio.ini build_flags if not already present
INCLUDE_FLAG="-include src/config/user_config.h"
if grep -qF "$INCLUDE_FLAG" "$PLATFORMIO_INI" 2>/dev/null; then
  echo "  -include flag already present in platformio.ini (skipping)"
else
  # Append after the last -I line in the build_flags section
  sed -i '/^    -I include$/a\    -include src/config/user_config.h' "$PLATFORMIO_INI"
  echo "  Added -include flag to platformio.ini"
fi

echo ""

# ---------------------------------------------------------------------------
# J. Summary output
# ---------------------------------------------------------------------------
echo "============================================="
echo " NodeAlert IoT — Deployment Complete!"
echo "============================================="
echo ""
echo "Services:"
echo "  MQTT Broker:   ${MQTT_BROKER_IP}:1883"
echo "  Django API:    http://${MQTT_BROKER_IP}:8000/api/v1/"
echo "  Django Admin:  http://${MQTT_BROKER_IP}:8000/admin/"
echo ""
echo "Management Commands:"
echo "  View logs:     docker compose logs -f"
echo "  Stop services: docker compose down"
echo "  Restart:       docker compose restart"
echo ""
echo "Firmware Configuration:"
echo "  Config file:   ${FIRMWARE_CONFIG_FILE}"
echo "  Re-run setup.sh to regenerate."
echo ""
echo "============================================="
echo " ⚠️  WARNING: .env contains sensitive credentials."
echo "    Keep this file secure and never commit it"
echo "    to version control (.env is in .gitignore)."
echo "============================================="
