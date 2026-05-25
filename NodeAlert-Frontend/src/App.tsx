import { Routes, Route, Navigate } from 'react-router-dom'
import { AuthProvider } from './context/AuthContext'
import { ReadingsProvider } from './context/ReadingsContext'
import { AlarmProvider } from './context/AlarmContext'
import LoginPage from './pages/LoginPage'
import DashboardPage from './pages/DashboardPage'
import DeviceListPage from './pages/DeviceListPage'
import DeviceDetailPage from './pages/DeviceDetailPage'
import ProtectedRoute from './components/ProtectedRoute'

export default function App() {
  return (
    <AuthProvider>
      <ReadingsProvider>
        <AlarmProvider>
          <Routes>
            <Route path="/login" element={<LoginPage />} />
            <Route
              path="/"
              element={
                <ProtectedRoute>
                  <DashboardPage />
                </ProtectedRoute>
              }
            />
            <Route
              path="/devices"
              element={
                <ProtectedRoute>
                  <DeviceListPage />
                </ProtectedRoute>
              }
            />
            <Route
              path="/device/:id"
              element={
                <ProtectedRoute>
                  <DeviceDetailPage />
                </ProtectedRoute>
              }
            />
            <Route path="*" element={<Navigate to="/" replace />} />
          </Routes>
        </AlarmProvider>
      </ReadingsProvider>
    </AuthProvider>
  )
}
