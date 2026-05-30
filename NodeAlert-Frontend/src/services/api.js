/**
 * Cliente HTTP centralizado para la API REST de NodeAlert.
 *
 * Configura axios con la URL base (/api/v1) e interceptores que:
 *   1. Adjuntan el token de autenticación desde localStorage a cada
 *      solicitud saliente (interceptor de request).
 *   2. Detectan respuestas 401 (no autorizado) y redirigen al login,
 *      limpiando el token almacenado (interceptor de response).
 *
 * Todas las funciones de API exportadas utilizan este cliente,
 * garantizando consistencia en headers, manejo de errores y
 * autenticación.
 */
import axios from 'axios'

const API_BASE = '/api/v1'

const api = axios.create({
  baseURL: API_BASE,
  headers: { 'Content-Type': 'application/json' },
})

/**
 * Interceptor de request: agrega el token de autenticación a todas
 * las solicitudes salientes si existe en localStorage.
 */
api.interceptors.request.use((config) => {
  const token = localStorage.getItem('token')
  if (token) {
    config.headers.Authorization = `Token ${token}`
  }
  return config
})

/**
 * Interceptor de response: maneja errores globales de autenticación.
 * Si el servidor responde con 401, se limpia el token y se redirige
 * al login para que el usuario vuelva a autenticarse.
 */
api.interceptors.response.use(
  (response) => response,
  (error) => {
    if (error.response?.status === 401) {
      localStorage.removeItem('token')
      window.location.href = '/login'
    }
    return Promise.reject(error)
  },
)

export async function loginUser(username, password) {
  const { data } = await api.post('/auth/login/', { username, password })
  return data
}

export async function getCurrentUser() {
  const { data } = await api.get('/auth/me')
  return data
}

export async function getDevices(params) {
  const { data } = await api.get('/devices', { params })
  return data
}

export async function getDevice(id) {
  const { data } = await api.get(`/devices/${id}`)
  return data
}

export async function getReadings(params) {
  const { data } = await api.get('/readings', { params })
  return data
}

export async function getEvents(params) {
  const { data } = await api.get('/events', { params })
  return data
}

export async function updateEvent(id, payload) {
  const { data } = await api.patch(`/events/${id}/`, payload)
  return data
}

export async function sendCommand(deviceId, command, params) {
  const { data } = await api.post(`/devices/${deviceId}/command/`, {
    command,
    params,
  })
  return data
}

export default api
