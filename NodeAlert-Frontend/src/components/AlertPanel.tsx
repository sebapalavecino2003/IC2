import { useState } from 'react'
import { Card, CardContent, Typography, Alert as MuiAlert } from '@mui/material'
import EventTable from './EventTable'
import { useReadings } from '../context/ReadingsContext'
import api from '../services/api'

export default function AlertPanel() {
  const { events } = useReadings()
  const [localEvents, setLocalEvents] = useState(events)

  const currentEvents = events.length > 0 ? events : localEvents

  const handleResolve = async (id: number) => {
    try {
      await api.patch(`/events/${id}/`, { resolved: true })
      setLocalEvents((prev) => prev.filter((e) => e.id !== id))
    } catch {
      // Silently fail
    }
  }

  return (
    <Card>
      <CardContent>
        <Typography variant="h6" gutterBottom>Eventos y Alertas</Typography>
        {currentEvents.length === 0 && (
          <MuiAlert severity="info">No hay eventos registrados</MuiAlert>
        )}
        <EventTable events={currentEvents} onResolve={handleResolve} />
      </CardContent>
    </Card>
  )
}
