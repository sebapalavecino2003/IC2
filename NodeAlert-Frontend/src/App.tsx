import { Routes, Route, Navigate } from 'react-router-dom'

export default function App() {
  return (
    <Routes>
      <Route path="/login" element={<div>Login Page</div>} />
      <Route path="/" element={<div>Dashboard</div>} />
      <Route path="*" element={<Navigate to="/" replace />} />
    </Routes>
  )
}
