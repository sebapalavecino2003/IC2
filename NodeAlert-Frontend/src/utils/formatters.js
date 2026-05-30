/**
 * Utilidades de formateo para la interfaz de NodeAlert.
 *
 * Centraliza las funciones de transformación de datos para presentación
 * en la UI: timestamps a formato legible, estado de dispositivos,
 * colores según severidad, y valores con unidades.
 *
 * Mantener estas funciones aquí evita duplicación de lógica de
 * presentación entre componentes y facilita cambios de formato
 * sin modificar múltiples archivos.
 */

/**
 * Formatea un timestamp ISO a fecha/hora local en español.
 * Ejemplo: "27/05/2026 14:30:15"
 */
export function formatTimestamp(isoStr) {
  if (!isoStr) return '—'
  const d = new Date(isoStr)
  return d.toLocaleString('es-ES', {
    day: '2-digit',
    month: '2-digit',
    year: 'numeric',
    hour: '2-digit',
    minute: '2-digit',
    second: '2-digit',
  })
}

/**
 * Calcula el tiempo transcurrido desde una fecha ISO hasta ahora.
 * Retorna un formato compacto: "3m 12s", "2h 30m", "5d".
 * Ideal para mostrar antigüedad de eventos en listas compactas.
 */
export function timeAgo(isoStr) {
  if (!isoStr) return '—'
  const diff = Date.now() - new Date(isoStr).getTime()
  const sec = Math.floor(diff / 1000)
  if (sec < 60) return `${sec}s`
  const min = Math.floor(sec / 60)
  if (min < 60) return `${min}m ${sec % 60}s`
  const hr = Math.floor(min / 60)
  if (hr < 24) return `${hr}h ${min % 60}m`
  return `${Math.floor(hr / 24)}d`
}

/**
 * Determina el estado de un sensor basado en su valor respecto al máximo.
 *   - normal: valor por debajo del 80%.
 *   - warning: entre 80% y 100%.
 *   - critical: igual o superior al 100%.
 *   - offline: valor nulo (sin lectura).
 */
export function sensorStatus(value, max) {
  if (value == null) return 'offline'
  const ratio = value / max
  if (ratio >= 1) return 'critical'
  if (ratio >= 0.8) return 'warning'
  return 'normal'
}

/**
 * Determina si un dispositivo está online basado en su última conexión.
 * Se considera online si la última conexión fue hace menos de 35 segundos.
 * Este umbral permite tolerar hasta 2 ciclos de telemetría perdidos
 * (considerando el intervalo de 15 segundos del firmware) antes de
 * marcar el dispositivo como offline.
 */
export function deviceStatus(isoStr) {
  if (!isoStr) return 'offline'
  const diff = Date.now() - new Date(isoStr).getTime()
  return diff < 35000 ? 'online' : 'offline'
}

/**
 * Retorna el color hexadecimal asociado a cada estado del sistema.
 * Los colores están alineados con el tema oscuro de la aplicación.
 */
export function statusColor(status) {
  const map = {
    online: '#00e676',
    offline: '#64748b',
    warning: '#ffd600',
    critical: '#ff1744',
    normal: '#00e676',
    info: '#2979ff',
  }
  return map[status] || '#64748b'
}

/**
 * Formatea un valor numérico con una unidad, redondeado a 1 decimal.
 * Ejemplo: formatValue(23.456, '°C') → "23.5 °C"
 */
export function formatValue(value, unit) {
  if (value == null) return '—'
  return `${Number(value).toFixed(1)} ${unit}`
}
