import { createContext, useContext, useState, useEffect, type ReactNode } from 'react'
import api from '../services/api'
import type { UserInfo } from '../types'

interface AuthState {
  token: string | null
  isAuthenticated: boolean
  isLoading: boolean
  error: string | null
  userInfo: UserInfo | null
  isAdmin: boolean
  login: (username: string, password: string) => Promise<void>
  logout: () => void
}

const AuthContext = createContext<AuthState | null>(null)

export function AuthProvider({ children }: { children: ReactNode }) {
  const [token, setToken] = useState<string | null>(() => localStorage.getItem('token'))
  const [userInfo, setUserInfo] = useState<UserInfo | null>(null)
  const [isLoading, setIsLoading] = useState(false)
  const [error, setError] = useState<string | null>(null)

  const isAuthenticated = token !== null
  const isAdmin = userInfo?.role === 'admin'

  const fetchUserInfo = async () => {
    try {
      const res = await api.get('/auth/me')
      setUserInfo(res.data)
    } catch {
      setUserInfo(null)
    }
  }

  const login = async (username: string, password: string) => {
    setIsLoading(true)
    setError(null)
    try {
      const res = await api.post('/auth/login', { username, password })
      const newToken = res.data.token
      localStorage.setItem('token', newToken)
      setToken(newToken)
      await fetchUserInfo()
    } catch (err: any) {
      const msg = err.response?.data?.detail || err.response?.data?.non_field_errors?.[0] || 'Error de autenticación'
      setError(msg)
      throw new Error(msg)
    } finally {
      setIsLoading(false)
    }
  }

  const logout = () => {
    localStorage.removeItem('token')
    setToken(null)
    setUserInfo(null)
  }

  useEffect(() => {
    if (token) {
      fetchUserInfo()
    }
  }, [])

  return (
    <AuthContext.Provider value={{ token, isAuthenticated, isLoading, error, userInfo, isAdmin, login, logout }}>
      {children}
    </AuthContext.Provider>
  )
}

export function useAuth() {
  const ctx = useContext(AuthContext)
  if (!ctx) throw new Error('useAuth must be used within AuthProvider')
  return ctx
}
