#!/bin/bash
# =============================================================================
# NodeAlert IoT — Deployment script (configuración hardcodeada)
# =============================================================================
# Este script despliega el stack NodeAlert IoT. Las credenciales están
# hardcodeadas en el código. Editá docs/contraseñas.md para cambiarlas.
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

Options:
  --help, -h    Show this help message and exit

Las credenciales están hardcodeadas en el código.
Editá docs/contraseñas.md para cambiarlas.
EOF
  exit 0
}

if [ "${1:-}" = "--help" ] || [ "${1:-}" = "-h" ]; then
  show_help
fi

# Credenciales hardcodeadas para el despliegue.
# En un entorno productivo, estas deberían leerse de variables de
# entorno o un vault de secretos.
MQTT_BROKER_USER="nodealert_gateway"
MQTT_BROKER_PASSWORD="test_password"
MQTT_BROKER_IP="10.1.1.10"
FIRMWARE_MQTT_USER="nodealert_esp32"
FIRMWARE_MQTT_PASSWORD="test_password"
FIRMWARE_DEVICE_ID="nodealert-01"
MQTT_SUBSCRIBER_USER="mqtt_subscriber"
MQTT_SUBSCRIBER_PASSWORD="test_password"
MYSQL_ROOT_PASSWORD="root_password"
MYSQL_DATABASE="nodealert"
MYSQL_USER="nodealert"
MYSQL_PASSWORD="db_password"
DJANGO_SUPERUSER_USERNAME="admin"
DJANGO_SUPERUSER_EMAIL="admin@nodealert.local"
DJANGO_SUPERUSER_PASSWORD="admin_password"
WIFI_SSID="IC-2.4G"
WIFI_PASS="1nf0rm4t1c4_2025"

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
echo "Usando credenciales hardcodeadas (ver docs/contraseñas.md)"
echo ""

# ---------------------------------------------------------------------------
# E. Generate mosquitto_passwd
# ---------------------------------------------------------------------------
echo "Generating Mosquitto password file..."

if command -v mosquitto_passwd &>/dev/null; then
  MOSQUITTO_PASSWD_DIR="mosquitto/config"
  MOSQUITTO_PASSWD_FILE="${MOSQUITTO_PASSWD_DIR}/mosquitto_passwd"

  mkdir -p "$MOSQUITTO_PASSWD_DIR"
  rm -f "$MOSQUITTO_PASSWD_FILE"

  if mosquitto_passwd -b "$MOSQUITTO_PASSWD_FILE" "$MQTT_BROKER_USER" "$MQTT_BROKER_PASSWORD"; then
    echo "  mosquitto_passwd generated at ${MOSQUITTO_PASSWD_FILE}"
  else
    echo "  ERROR: mosquitto_passwd command failed."
    exit 1
  fi

  # Add firmware MQTT user (D-19)
  if mosquitto_passwd -b "$MOSQUITTO_PASSWD_FILE" "$FIRMWARE_MQTT_USER" "$FIRMWARE_MQTT_PASSWORD"; then
    echo "  Added firmware MQTT user: ${FIRMWARE_MQTT_USER}"
  else
    echo "  WARNING: Failed to add firmware MQTT user (non-fatal)."
  fi

  # Add MQTT subscriber user (D-17)
  if mosquitto_passwd -b "$MOSQUITTO_PASSWD_FILE" "$MQTT_SUBSCRIBER_USER" "$MQTT_SUBSCRIBER_PASSWORD"; then
    echo "  Added MQTT subscriber user: ${MQTT_SUBSCRIBER_USER}"
  else
    echo "  WARNING: Failed to add subscriber user (non-fatal)."
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

  # Add firmware MQTT user (D-19)
  if mosquitto_passwd -b "$MOSQUITTO_PASSWD_FILE" "$FIRMWARE_MQTT_USER" "$FIRMWARE_MQTT_PASSWORD"; then
    echo "  Added firmware MQTT user: ${FIRMWARE_MQTT_USER}"
  else
    echo "  WARNING: Failed to add firmware MQTT user (non-fatal)."
  fi

  # Add MQTT subscriber user (D-17)
  if mosquitto_passwd -b "$MOSQUITTO_PASSWD_FILE" "$MQTT_SUBSCRIBER_USER" "$MQTT_SUBSCRIBER_PASSWORD"; then
    echo "  Added MQTT subscriber user: ${MQTT_SUBSCRIBER_USER}"
  else
    echo "  WARNING: Failed to add subscriber user (non-fatal)."
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

FIRMWARE_MQTT_PASS="${FIRMWARE_MQTT_PASSWORD}"

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

/* ------------------------------------------------------------------------ */
/* MQTT Credentials (overrides mqtt_config.h defaults — D-18/D-19)          */
/* ------------------------------------------------------------------------ */
#define MQTT_USER               "${FIRMWARE_MQTT_USER}"
#define MQTT_PASS               "${FIRMWARE_MQTT_PASS}"

/* ------------------------------------------------------------------------ */
/* Device Identity (overrides mqtt_config.h defaults)                       */
/* ------------------------------------------------------------------------ */
#define DEVICE_ID               "${FIRMWARE_DEVICE_ID}"
CONFIGEOF

echo "  Firmware config generated: ${FIRMWARE_CONFIG_FILE}"

# Add -include compiler flag to platformio.ini build_flags if not already present
INCLUDE_FLAG="-include src/config/user_config.h"
if grep -qF "$INCLUDE_FLAG" "$PLATFORMIO_INI" 2>/dev/null; then
  echo "  -include flag already present in platformio.ini (skipping)"
else
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
echo "MQTT Subscriber:"
echo "  Subscriber user:  ${MQTT_SUBSCRIBER_USER}"
echo "  Firmware user:    ${FIRMWARE_MQTT_USER}"
echo "  Device ID:        ${FIRMWARE_DEVICE_ID}"
echo ""
echo "Management Commands:"
echo "  View logs:     docker compose logs -f"
echo "  Stop services: docker compose down"
echo "  Restart:       docker compose restart"
echo "  MQTT subscriber auto-starts with Django container"
echo ""
echo "Firmware Configuration:"
echo "  Config file:   ${FIRMWARE_CONFIG_FILE}"
echo "  Re-run setup.sh to regenerate."
echo ""
echo "============================================="
echo " Las credenciales están hardcodeadas en el código."
echo " Editá docs/contraseñas.md para modificarlas."
echo "============================================="
