import { Navigate } from 'react-router-dom'
import { useAuth } from '../context/AuthContext'

interface Props {
  children: React.ReactNode
  requiredRole?: 'admin'
}

export default function ProtectedRoute({ children, requiredRole }: Props) {
  const { isAuthenticated, isAdmin } = useAuth()
  if (!isAuthenticated) return <Navigate to="/login" replace />
  if (requiredRole === 'admin' && !isAdmin) return <Navigate to="/" replace />
  return <>{children}</>
}
