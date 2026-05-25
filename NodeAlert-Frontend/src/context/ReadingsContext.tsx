import { createContext, useContext, useState, useCallback, type ReactNode } from 'react'
import { useAuth } from './AuthContext'
import { usePolling } from '../hooks/usePolling'
import { fetchLatestReadings, fetchUnresolvedEvents } from '../services/polling'
import type { Reading, Event as AlertEvent } from '../types'

interface ReadingsState {
  readings: Reading[]
  events: AlertEvent[]
  isPolling: boolean
  lastUpdate: string | null
}

const ReadingsContext = createContext<ReadingsState | null>(null)

export function ReadingsProvider({ children }: { children: ReactNode }) {
  const { isAuthenticated } = useAuth()
  const [readings, setReadings] = useState<Reading[]>([])
  const [events, setEvents] = useState<AlertEvent[]>([])
  const [lastUpdate, setLastUpdate] = useState<string | null>(null)

  const poll = useCallback(async () => {
    try {
      const [newReadings, newEvents] = await Promise.all([
        fetchLatestReadings(),
        fetchUnresolvedEvents(),
      ])
      setReadings(newReadings)
      setEvents(newEvents)
      setLastUpdate(new Date().toISOString())
    } catch {
      // Silently fail — api.ts interceptor handles 401
    }
  }, [])

  usePolling(poll, 3000, isAuthenticated)

  return (
    <ReadingsContext.Provider value={{ readings, events, isPolling: isAuthenticated, lastUpdate }}>
      {children}
    </ReadingsContext.Provider>
  )
}

export function useReadings() {
  const ctx = useContext(ReadingsContext)
  if (!ctx) throw new Error('useReadings must be used within ReadingsProvider')
  return ctx
}
