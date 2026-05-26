import { useState } from 'react'
import { Card, CardContent, Typography, Tabs, Tab } from '@mui/material'
import EventTable from './EventTable'
import ActiveAlertsPanel from './ActiveAlertsPanel'
import { useReadings } from '../context/ReadingsContext'
import api from '../services/api'

export default function AlertPanel() {
  const { events } = useReadings()
  const [tab, setTab] = useState(0)
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
        <Typography variant="h6" gutterBottom>Alertas y Eventos</Typography>
        <Tabs value={tab} onChange={(_, v) => setTab(v)} sx={{ mb: 2 }}>
          <Tab label="Alertas Activas" />
          <Tab label="Historial" />
        </Tabs>
        {tab === 0 && <ActiveAlertsPanel />}
        {tab === 1 && <EventTable events={currentEvents} onResolve={handleResolve} />}
      </CardContent>
    </Card>
  )
}
