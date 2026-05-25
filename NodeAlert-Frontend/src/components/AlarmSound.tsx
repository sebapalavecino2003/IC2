import { useEffect, useRef } from 'react'
import { useAlarm } from '../context/AlarmContext'

export default function AlarmSound() {
  const { alarmActive } = useAlarm()
  const audioRef = useRef<HTMLAudioElement | null>(null)

  useEffect(() => {
    if (!audioRef.current) {
      audioRef.current = new Audio('data:audio/wav;base64,UklGRnoGAABXQVZFZm10IBAAAAABAAEAQB8AAEAfAAABAAgAZGF0YQoGAACAf39/f4B/f3+AgH9/f3+AgH9/f3+AgH9/f3+AgH9/f3+AgH9/f3+AgH9/f3+AgH9/f3+AgH9/f3+AgH9/f3+AgH9/f3+AgH9/f3+AgH9/f3+AgH9/f3+AgH9/f3+AgH9/f3+AgH9/f38=')
      audioRef.current.loop = true
      audioRef.current.volume = 0.3
    }

    if (alarmActive) {
      audioRef.current.play().catch(() => {})
    } else {
      audioRef.current.pause()
      audioRef.current.currentTime = 0
    }

    return () => {
      if (audioRef.current) {
        audioRef.current.pause()
      }
    }
  }, [alarmActive])

  return null
}
