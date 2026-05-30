#!/bin/bash
# =============================================================================
# NodeAlert IoT — Deployment script
# =============================================================================
# Lee configuración desde .env (si existe) o usa valores por defecto.
# Copiar .env.example a .env y ajustar las variables antes de ejecutar.
#
# Usage: ./setup.sh [OPTIONS]
#   --help, -h    Show this help message and exit
#   --interactive, -i  Ask for IPs and passwords interactively
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
  --help, -h           Show this help message and exit
  --interactive, -i    Ask for IPs and passwords interactively

Without flags: reads from .env if exists, otherwise uses defaults.
EOF
  exit 0
}

INTERACTIVE=false
for arg in "$@"; do
  case "$arg" in
    --help|-h) show_help ;;
    --interactive|-i) INTERACTIVE=true ;;
  esac
done

# ---------------------------------------------------------------------------
# B. Cargar configuración desde .env o interactivo
# ---------------------------------------------------------------------------
ENV_FILE="$(dirname "$0")/.env"

_prompt() {
  local var="$1" msg="$2" default="$3"
  read -rp "$msg [$default]: " val
  echo "${val:-$default}"
}

if [ "$INTERACTIVE" = true ]; then
  echo "=== Configuración interactiva de NodeAlert ==="
  echo "Presioná Enter para usar el valor por defecto."
  echo ""

  MQTT_BROKER_IP=$(_prompt "MQTT_BROKER_IP" "IP del broker MQTT (la ven los ESP32)" "10.1.1.10")
  MQTT_GATEWAY_PASS=$(_prompt "MQTT_GATEWAY_PASS" "Contraseña gateway MQTT" "test_password")
  MQTT_FIRMWARE_PASS=$(_prompt "MQTT_FIRMWARE_PASS" "Contraseña firmware ESP32" "test_password")
  WIFI_SSID=$(_prompt "WIFI_SSID" "Red WiFi para ESP32" "IC-2.4G")
  WIFI_PASS=$(_prompt "WIFI_PASS" "Contraseña WiFi" "1nf0rm4t1c4_2025")
  MYSQL_ROOT_PASSWORD=$(_prompt "MYSQL_ROOT_PASSWORD" "Contraseña root MySQL" "root_password")
  MYSQL_PASSWORD=$(_prompt "MYSQL_PASSWORD" "Contraseña usuario MySQL" "db_password")
  DJANGO_SUPERUSER_USERNAME=$(_prompt "DJANGO_SUPERUSER_USERNAME" "Usuario admin Django" "admin")
  DJANGO_SUPERUSER_PASSWORD=$(_prompt "DJANGO_SUPERUSER_PASSWORD" "Contraseña admin Django" "admin_password")
  echo ""

  # Guardar en .env para reuso
  cat > "$ENV_FILE" <<EOF
# NodeAlert IoT — Configuración generada por setup.sh --interactive
WIFI_SSID=${WIFI_SSID}
WIFI_PASS=${WIFI_PASS}
MQTT_BROKER_IP=${MQTT_BROKER_IP}
MQTT_GATEWAY_PASS=${MQTT_GATEWAY_PASS}
MQTT_FIRMWARE_PASS=${MQTT_FIRMWARE_PASS}
MYSQL_ROOT_PASSWORD=${MYSQL_ROOT_PASSWORD}
MYSQL_PASSWORD=${MYSQL_PASSWORD}
DJANGO_SUPERUSER_USERNAME=${DJANGO_SUPERUSER_USERNAME}
DJANGO_SUPERUSER_PASSWORD=${DJANGO_SUPERUSER_PASSWORD}
EOF
  echo "Configuración guardada en ${ENV_FILE}"
  echo ""
elif [ -f "$ENV_FILE" ]; then
  echo "Cargando configuración desde ${ENV_FILE}"
  set -a
  source "$ENV_FILE"
  set +a
else
  echo "No se encontró .env — usando valores por defecto"
  echo "Ejecutá con --interactive o -i para configurar:"
  echo "  ./setup.sh -i"
  echo ""
fi

# Valores por defecto (usados si no están definidos en .env)
MQTT_BROKER_USER="${MQTT_GATEWAY_USER:-nodealert_gateway}"
MQTT_BROKER_PASSWORD="${MQTT_GATEWAY_PASS:-test_password}"
MQTT_BROKER_IP="${MQTT_BROKER_IP:-10.1.1.10}"
FIRMWARE_MQTT_USER="${MQTT_FIRMWARE_USER:-nodealert_esp32}"
FIRMWARE_MQTT_PASSWORD="${MQTT_FIRMWARE_PASS:-test_password}"
FIRMWARE_DEVICE_ID="${FIRMWARE_DEVICE_ID:-nodealert-01}"
MQTT_SUBSCRIBER_USER="${MQTT_SUBSCRIBER_USER:-mqtt_subscriber}"
MQTT_SUBSCRIBER_PASSWORD="${MQTT_SUBSCRIBER_PASS:-test_password}"
MQTT_PUBLISHER_USER="${MQTT_PUBLISHER_USER:-mqtt_publisher}"
MQTT_PUBLISHER_PASSWORD="${MQTT_PUBLISHER_PASS:-test_password}"
MYSQL_ROOT_PASSWORD="${MYSQL_ROOT_PASSWORD:-root_password}"
MYSQL_DATABASE="${MYSQL_DATABASE:-nodealert}"
MYSQL_USER="${MYSQL_USER:-nodealert}"
MYSQL_PASSWORD="${MYSQL_PASSWORD:-db_password}"
DJANGO_SUPERUSER_USERNAME="${DJANGO_SUPERUSER_USERNAME:-admin}"
DJANGO_SUPERUSER_EMAIL="${DJANGO_SUPERUSER_EMAIL:-admin@nodealert.local}"
DJANGO_SUPERUSER_PASSWORD="${DJANGO_SUPERUSER_PASSWORD:-admin_password}"
WIFI_SSID="${WIFI_SSID:-IC-2.4G}"
WIFI_PASS="${WIFI_PASS:-1nf0rm4t1c4_2025}"

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
echo "Usando configuración de ${ENV_FILE:-valores por defecto}"
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

  # Add MQTT publisher user
  if mosquitto_passwd -b "$MOSQUITTO_PASSWD_FILE" "$MQTT_PUBLISHER_USER" "$MQTT_PUBLISHER_PASSWORD"; then
    echo "  Added MQTT publisher user: ${MQTT_PUBLISHER_USER}"
  else
    echo "  WARNING: Failed to add publisher user (non-fatal)."
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

  # Add MQTT publisher user
  if mosquitto_passwd -b "$MOSQUITTO_PASSWD_FILE" "$MQTT_PUBLISHER_USER" "$MQTT_PUBLISHER_PASSWORD"; then
    echo "  Added MQTT publisher user: ${MQTT_PUBLISHER_USER}"
  else
    echo "  WARNING: Failed to add publisher user (non-fatal)."
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

FIRMWARE_CONFIG_DIR="Molotica/src/config"
FIRMWARE_CONFIG_FILE="${FIRMWARE_CONFIG_DIR}/user_config.h"
PLATFORMIO_INI="Molotica/platformio.ini"

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
 * Included before Config.h via -include compiler flag in platformio.ini.
 * Overrides the #ifndef defaults in Molotica/src/Config.h.
 */

#pragma once

/* ------------------------------------------------------------------------ */
/* MQTT Broker                                                              */
/* ------------------------------------------------------------------------ */
#ifndef MQTT_BROKER_IP
#define MQTT_BROKER_IP          "${MQTT_BROKER_IP}"
#endif

/* ------------------------------------------------------------------------ */
/* MQTT Credentials                                                         */
/* ------------------------------------------------------------------------ */
#ifndef USUARIO_MQTT
#define USUARIO_MQTT            "${FIRMWARE_MQTT_USER}"
#endif
#ifndef CLAVE_MQTT
#define CLAVE_MQTT              "${FIRMWARE_MQTT_PASS}"
#endif

/* ------------------------------------------------------------------------ */
/* Device Identity                                                          */
/* ------------------------------------------------------------------------ */
#ifndef ID_DISPOSITIVO
#define ID_DISPOSITIVO          "${FIRMWARE_DEVICE_ID}"
#endif
CONFIGEOF

echo "  Firmware config generated: ${FIRMWARE_CONFIG_FILE}"

# Add -include compiler flag to platformio.ini build_flags if not already present
INCLUDE_FLAG="-include src/config/user_config.h"
if [ -f "$PLATFORMIO_INI" ]; then
  if grep -qF "$INCLUDE_FLAG" "$PLATFORMIO_INI" 2>/dev/null; then
    echo "  -include flag already present in platformio.ini (skipping)"
  else
    sed -i '/^build_flags =/a\    -include src/config/user_config.h' "$PLATFORMIO_INI"
    echo "  Added -include flag to platformio.ini"
  fi
else
  echo "  WARNING: ${PLATFORMIO_INI} not found (firmware build config unchanged)"
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
echo "MQTT Users:"
echo "  Gateway:   ${MQTT_BROKER_USER}"
echo "  Publisher: ${MQTT_PUBLISHER_USER}"
echo "  Subscriber: ${MQTT_SUBSCRIBER_USER}"
echo "  Firmware:  ${FIRMWARE_MQTT_USER}"
echo ""
echo "Firmware:"
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
echo " Configuración lista."
echo " Editá .env para modificar credenciales y IPs."
echo " Luego ejecutá ./setup.sh nuevamente."
echo "============================================="
