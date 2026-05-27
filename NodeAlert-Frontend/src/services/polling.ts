import api from './api'
import type { Reading, Event as AlertEvent, PaginatedResponse } from '../types'

export interface PollingResult {
  latestReadings: Reading[]
  events: AlertEvent[]
}

export async function fetchLatestReadings(): Promise<Reading[]> {
  const res = await api.get<PaginatedResponse<Reading>>('/readings', {
    params: { ordering: '-timestamp' },
  })
  return res.data.results
}

export async function fetchUnresolvedEvents(): Promise<AlertEvent[]> {
  const res = await api.get<PaginatedResponse<AlertEvent>>('/events', {
    params: { resolved: false, ordering: '-timestamp' },
  })
  return res.data.results
}
