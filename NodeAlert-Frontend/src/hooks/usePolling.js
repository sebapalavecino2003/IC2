/**
 * Hook personalizado para polling periódico de datos.
 *
 * Ejecuta un callback a intervalos regulares mientras la aplicación
 * está activa. Utiliza useRef para mantener la referencia actualizada
 * al callback sin reiniciar el intervalo, evitando fugas de memoria
 * y comportamientos inconsistentes cuando el callback cambia.
 *
 * @param {Function} callback  Función a ejecutar en cada ciclo.
 * @param {number}   interval  Tiempo entre ejecuciones en milisegundos.
 * @param {boolean}  enabled   Controla si el polling está activo.
 */
import { useEffect, useRef } from 'react'

export default function usePolling(callback, interval = 3000, enabled = true) {
  const savedCallback = useRef(callback)

  // Mantener la referencia al callback siempre actualizada.
  useEffect(() => {
    savedCallback.current = callback
  }, [callback])

  // Configurar el intervalo solo si está habilitado.
  useEffect(() => {
    if (!enabled) return
    savedCallback.current()
    const id = setInterval(() => savedCallback.current(), interval)
    return () => clearInterval(id)
  }, [interval, enabled])
}
