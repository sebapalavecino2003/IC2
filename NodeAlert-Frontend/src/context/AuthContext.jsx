/**
 * Contexto de autenticación de NodeAlert.
 *
 * Gestiona el estado de autenticación del usuario en toda la aplicación.
 * En el montaje inicial, verifica si hay un token almacenado y recupera
 * la información del usuario desde la API. Proporciona funciones login
 * y logout a todos los componentes hijos sin prop drilling.
 *
 * El estado 'loading' permite a componentes como ProtectedRoute mostrar
 * un indicador de carga mientras se verifica la sesión existente.
 */
import { createContext, useContext, useState, useEffect, useCallback } from 'react'
import { loginUser, getCurrentUser } from '../services/api'

const AuthContext = createContext(null)

export function AuthProvider({ children }) {
  const [user, setUser] = useState(null)
  const [loading, setLoading] = useState(true)

  /**
   * Al montar el provider, verifica si existe un token previo.
   * Si existe, intenta recuperar los datos del usuario; si falla
   * (token inválido/expirado), limpia el token y establece user=null.
   */
  useEffect(() => {
    const token = localStorage.getItem('token')
    if (token) {
      getCurrentUser()
        .then(setUser)
        .catch(() => {
          localStorage.removeItem('token')
          setUser(null)
        })
        .finally(() => setLoading(false))
    } else {
      setLoading(false)
    }
  }, [])

  /**
   * login: autentica al usuario, almacena el token y obtiene los
   * datos del usuario autenticado.
   */
  const login = useCallback(async (username, password) => {
    const data = await loginUser(username, password)
    localStorage.setItem('token', data.token)
    const userData = { ...data }
    delete userData.token
    setUser(userData)
    return userData
  }, [])

  /**
   * logout: elimina el token y resetea el estado del usuario.
   */
  const logout = useCallback(() => {
    localStorage.removeItem('token')
    setUser(null)
  }, [])

  return (
    <AuthContext.Provider value={{ user, loading, login, logout }}>
      {children}
    </AuthContext.Provider>
  )
}

/**
 * Hook personalizado para acceder al contexto de autenticación.
 * Lanza un error si se usa fuera del AuthProvider.
 */
export function useAuth() {
  const ctx = useContext(AuthContext)
  if (!ctx) throw new Error('useAuth must be inside AuthProvider')
  return ctx
}
