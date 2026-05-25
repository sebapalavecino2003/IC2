import api from './api'
import type { Reading, Event as AlertEvent } from '../types'

export interface PollingResult {
  latestReadings: Reading[]
  events: AlertEvent[]
}

export async function fetchLatestReadings(): Promise<Reading[]> {
  const res = await api.get<Reading[]>('/readings', {
    params: { ordering: '-timestamp', limit: 50 },
  })
  return res.data
}

export async function fetchUnresolvedEvents(): Promise<AlertEvent[]> {
  const res = await api.get<AlertEvent[]>('/events', {
    params: { resolved: false, ordering: '-timestamp', limit: 10 },
  })
  return res.data
}
