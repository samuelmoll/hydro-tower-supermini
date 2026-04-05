<template>
  <div class="space-y-6">
    <!-- Page header -->
    <div class="flex justify-between items-center">
      <h1 class="text-2xl font-bold text-gray-900 dark:text-white">Sensor History</h1>
      <select v-model="timeRange" @change="fetchData" class="input w-auto">
        <option value="6">Last 6 hours</option>
        <option value="24">Last 24 hours</option>
        <option value="48">Last 48 hours</option>
        <option value="168">Last 7 days</option>
      </select>
    </div>

    <!-- Charts -->
    <div class="grid grid-cols-1 lg:grid-cols-2 gap-6">
      <!-- Temperature chart -->
      <div class="card">
        <h2 class="text-lg font-semibold text-gray-900 dark:text-white mb-4">
          Temperature (°C)
        </h2>
        <div class="h-64">
          <Line v-if="chartData.temperature" :data="chartData.temperature" :options="chartOptions" />
          <p v-else class="text-gray-500 text-center py-8">Loading chart...</p>
        </div>
      </div>

      <!-- Humidity chart -->
      <div class="card">
        <h2 class="text-lg font-semibold text-gray-900 dark:text-white mb-4">
          Humidity (%)
        </h2>
        <div class="h-64">
          <Line v-if="chartData.humidity" :data="chartData.humidity" :options="chartOptions" />
          <p v-else class="text-gray-500 text-center py-8">Loading chart...</p>
        </div>
      </div>
    </div>

    <!-- Recent readings table -->
    <div class="card">
      <h2 class="text-lg font-semibold text-gray-900 dark:text-white mb-4">
        Recent Readings
      </h2>
      <div class="overflow-x-auto">
        <table class="w-full">
          <thead>
            <tr class="border-b border-gray-200 dark:border-gray-700">
              <th class="text-left py-3 px-4 text-sm font-medium text-gray-500 dark:text-gray-400">
                Time
              </th>
              <th class="text-right py-3 px-4 text-sm font-medium text-gray-500 dark:text-gray-400">
                Temperature
              </th>
              <th class="text-right py-3 px-4 text-sm font-medium text-gray-500 dark:text-gray-400">
                Humidity
              </th>
            </tr>
          </thead>
          <tbody>
            <tr
              v-for="reading in recentReadings"
              :key="reading.id"
              class="border-b border-gray-100 dark:border-gray-800"
            >
              <td class="py-3 px-4 text-sm text-gray-600 dark:text-gray-300">
                {{ formatDateTime(reading.timestamp) }}
              </td>
              <td class="py-3 px-4 text-sm text-right text-gray-900 dark:text-white">
                {{ reading.temperature.toFixed(1) }}°C
              </td>
              <td class="py-3 px-4 text-sm text-right text-gray-900 dark:text-white">
                {{ reading.humidity.toFixed(1) }}%
              </td>
            </tr>
          </tbody>
        </table>
      </div>
      <p v-if="recentReadings.length === 0" class="text-center py-8 text-gray-500">
        No readings available
      </p>
    </div>
  </div>
</template>

<script>
import { ref, reactive, onMounted } from 'vue'
import { Line } from 'vue-chartjs'
import {
  Chart as ChartJS,
  CategoryScale,
  LinearScale,
  PointElement,
  LineElement,
  Title,
  Tooltip,
  Legend,
  Filler
} from 'chart.js'
import api from '../api/client'

// Register Chart.js components
ChartJS.register(
  CategoryScale,
  LinearScale,
  PointElement,
  LineElement,
  Title,
  Tooltip,
  Legend,
  Filler
)

export default {
  name: 'History',
  components: { Line },
  setup() {
    const timeRange = ref('24')
    const recentReadings = ref([])
    const chartData = reactive({
      temperature: null,
      humidity: null
    })

    const chartOptions = {
      responsive: true,
      maintainAspectRatio: false,
      plugins: {
        legend: { display: false }
      },
      scales: {
        x: {
          grid: { display: false }
        },
        y: {
          beginAtZero: false
        }
      }
    }

    const fetchData = async () => {
      try {
        const hours = parseInt(timeRange.value)
        const startDate = new Date()
        startDate.setHours(startDate.getHours() - hours)

        const response = await api.getSensors({
          start_date: startDate.toISOString(),
          limit: 500
        })

        const readings = response.data.reverse() // Oldest first for charts
        recentReadings.value = response.data.slice(0, 20) // Newest first for table

        // Prepare chart data
        const labels = readings.map(r => formatTime(r.timestamp))
        
        chartData.temperature = {
          labels,
          datasets: [{
            label: 'Temperature',
            data: readings.map(r => r.temperature),
            borderColor: '#f97316',
            backgroundColor: 'rgba(249, 115, 22, 0.1)',
            fill: true,
            tension: 0.3
          }]
        }

        chartData.humidity = {
          labels,
          datasets: [{
            label: 'Humidity',
            data: readings.map(r => r.humidity),
            borderColor: '#3b82f6',
            backgroundColor: 'rgba(59, 130, 246, 0.1)',
            fill: true,
            tension: 0.3
          }]
        }
      } catch (error) {
        console.error('Failed to fetch sensor data:', error)
      }
    }

    const formatTime = (timestamp) => {
      const date = new Date(timestamp)
      return date.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' })
    }

    const formatDateTime = (timestamp) => {
      const date = new Date(timestamp)
      return date.toLocaleString([], {
        month: 'short',
        day: 'numeric',
        hour: '2-digit',
        minute: '2-digit'
      })
    }

    onMounted(fetchData)

    return {
      timeRange,
      recentReadings,
      chartData,
      chartOptions,
      fetchData,
      formatDateTime
    }
  }
}
</script>
