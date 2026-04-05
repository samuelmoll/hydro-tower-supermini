<template>
  <div class="space-y-6">
    <!-- Page header -->
    <div class="flex justify-between items-center">
      <h1 class="text-2xl font-bold text-gray-900 dark:text-white">Dashboard</h1>
      <button @click="refresh" class="btn btn-secondary flex items-center">
        <svg class="w-4 h-4 mr-2" fill="none" stroke="currentColor" viewBox="0 0 24 24">
          <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" 
                d="M4 4v5h.582m15.356 2A8.001 8.001 0 004.582 9m0 0H9m11 11v-5h-.581m0 0a8.003 8.003 0 01-15.357-2m15.357 2H15" />
        </svg>
        Refresh
      </button>
    </div>

    <!-- Status cards -->
    <div class="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-6">
      <!-- Temperature card -->
      <div class="card">
        <div class="flex items-center justify-between">
          <div>
            <p class="text-sm font-medium text-gray-500 dark:text-gray-400">Temperature</p>
            <p class="text-3xl font-bold text-gray-900 dark:text-white">
              {{ latestReading?.temperature?.toFixed(1) ?? '--' }}°C
            </p>
          </div>
          <div class="p-3 bg-orange-100 dark:bg-orange-900 rounded-full">
            <span class="text-2xl">🌡️</span>
          </div>
        </div>
      </div>

      <!-- Humidity card -->
      <div class="card">
        <div class="flex items-center justify-between">
          <div>
            <p class="text-sm font-medium text-gray-500 dark:text-gray-400">Humidity</p>
            <p class="text-3xl font-bold text-gray-900 dark:text-white">
              {{ latestReading?.humidity?.toFixed(1) ?? '--' }}%
            </p>
          </div>
          <div class="p-3 bg-blue-100 dark:bg-blue-900 rounded-full">
            <span class="text-2xl">💧</span>
          </div>
        </div>
      </div>

      <!-- Pump status card -->
      <div class="card">
        <div class="flex items-center justify-between">
          <div>
            <p class="text-sm font-medium text-gray-500 dark:text-gray-400">Pump Status</p>
            <p class="text-3xl font-bold" :class="pumpIsOn ? 'text-green-500' : 'text-gray-400'">
              {{ pumpIsOn ? 'ON' : 'OFF' }}
            </p>
          </div>
          <div class="p-3 rounded-full" :class="pumpIsOn ? 'bg-green-100 dark:bg-green-900' : 'bg-gray-100 dark:bg-gray-700'">
            <span class="text-2xl">💦</span>
          </div>
        </div>
      </div>

      <!-- Uptime card -->
      <div class="card">
        <div class="flex items-center justify-between">
          <div>
            <p class="text-sm font-medium text-gray-500 dark:text-gray-400">Uptime</p>
            <p class="text-3xl font-bold text-gray-900 dark:text-white">
              {{ formatUptime(deviceStatus?.uptime_sec) }}
            </p>
          </div>
          <div class="p-3 bg-purple-100 dark:bg-purple-900 rounded-full">
            <span class="text-2xl">⏱️</span>
          </div>
        </div>
      </div>
    </div>

    <!-- Pump control section -->
    <div class="card">
      <h2 class="text-lg font-semibold text-gray-900 dark:text-white mb-4">Pump Control</h2>
      <div class="flex flex-wrap gap-3">
        <button
          @click="controlPump('on')"
          :disabled="controlLoading"
          class="btn btn-primary flex items-center"
        >
          <span class="mr-2">▶️</span> Turn On (5 min)
        </button>
        <button
          @click="controlPump('off')"
          :disabled="controlLoading"
          class="btn btn-danger flex items-center"
        >
          <span class="mr-2">⏹️</span> Turn Off
        </button>
        <button
          @click="controlPump('auto')"
          :disabled="controlLoading"
          class="btn btn-secondary flex items-center"
        >
          <span class="mr-2">🔄</span> Auto Mode
        </button>
      </div>
      <p v-if="controlMessage" class="mt-3 text-sm" :class="controlSuccess ? 'text-green-600' : 'text-red-600'">
        {{ controlMessage }}
      </p>
    </div>

    <!-- 24h Stats -->
    <div class="card">
      <h2 class="text-lg font-semibold text-gray-900 dark:text-white mb-4">24-Hour Statistics</h2>
      <div class="grid grid-cols-2 md:grid-cols-4 gap-4" v-if="stats">
        <div>
          <p class="text-sm text-gray-500 dark:text-gray-400">Avg Temperature</p>
          <p class="text-xl font-semibold text-gray-900 dark:text-white">
            {{ stats.temperature.avg?.toFixed(1) ?? '--' }}°C
          </p>
        </div>
        <div>
          <p class="text-sm text-gray-500 dark:text-gray-400">Temp Range</p>
          <p class="text-xl font-semibold text-gray-900 dark:text-white">
            {{ stats.temperature.min?.toFixed(1) ?? '--' }} - {{ stats.temperature.max?.toFixed(1) ?? '--' }}°C
          </p>
        </div>
        <div>
          <p class="text-sm text-gray-500 dark:text-gray-400">Avg Humidity</p>
          <p class="text-xl font-semibold text-gray-900 dark:text-white">
            {{ stats.humidity.avg?.toFixed(1) ?? '--' }}%
          </p>
        </div>
        <div>
          <p class="text-sm text-gray-500 dark:text-gray-400">Readings</p>
          <p class="text-xl font-semibold text-gray-900 dark:text-white">
            {{ stats.reading_count ?? 0 }}
          </p>
        </div>
      </div>
      <p v-else class="text-gray-500 dark:text-gray-400">Loading statistics...</p>
    </div>
  </div>
</template>

<script>
import { ref, computed, onMounted } from 'vue'
import api from '../api/client'

export default {
  name: 'Dashboard',
  props: {
    deviceStatus: Object
  },
  emits: ['refresh'],
  setup(props, { emit }) {
    const latestReading = ref(null)
    const stats = ref(null)
    const controlLoading = ref(false)
    const controlMessage = ref('')
    const controlSuccess = ref(false)

    const pumpIsOn = computed(() => props.deviceStatus?.pump_state === 'on')

    const refresh = async () => {
      emit('refresh')
      await Promise.all([
        fetchLatestReading(),
        fetchStats()
      ])
    }

    const fetchLatestReading = async () => {
      try {
        const response = await api.getLatestSensor()
        latestReading.value = response.data
      } catch (error) {
        console.error('Failed to fetch reading:', error)
      }
    }

    const fetchStats = async () => {
      try {
        const response = await api.getSensorStats(24)
        stats.value = response.data
      } catch (error) {
        console.error('Failed to fetch stats:', error)
      }
    }

    const controlPump = async (action) => {
      controlLoading.value = true
      controlMessage.value = ''
      try {
        const response = await api.controlPump(action, action === 'on' ? 5 : null)
        controlMessage.value = response.data.message
        controlSuccess.value = response.data.success
        emit('refresh')
      } catch (error) {
        controlMessage.value = error.response?.data?.detail || 'Failed to control pump'
        controlSuccess.value = false
      } finally {
        controlLoading.value = false
      }
    }

    const formatUptime = (seconds) => {
      if (!seconds) return '--'
      const hours = Math.floor(seconds / 3600)
      const minutes = Math.floor((seconds % 3600) / 60)
      if (hours > 0) return `${hours}h ${minutes}m`
      return `${minutes}m`
    }

    onMounted(refresh)

    return {
      latestReading,
      stats,
      pumpIsOn,
      controlLoading,
      controlMessage,
      controlSuccess,
      refresh,
      controlPump,
      formatUptime
    }
  }
}
</script>
