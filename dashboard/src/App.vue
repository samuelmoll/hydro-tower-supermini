<template>
  <div class="min-h-screen bg-gray-50 dark:bg-gray-900">
    <!-- Navigation -->
    <nav class="bg-white dark:bg-gray-800 shadow-sm">
      <div class="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8">
        <div class="flex justify-between h-16">
          <!-- Logo and title -->
          <div class="flex items-center">
            <span class="text-2xl">🌱</span>
            <span class="ml-2 text-xl font-bold text-gray-900 dark:text-white">
              Hydroponic Tower
            </span>
          </div>

          <!-- Navigation links -->
          <div class="flex items-center space-x-4">
            <router-link
              v-for="link in navLinks"
              :key="link.path"
              :to="link.path"
              class="px-3 py-2 rounded-md text-sm font-medium transition-colors"
              :class="[
                $route.path === link.path
                  ? 'bg-hydro-100 text-hydro-700 dark:bg-hydro-900 dark:text-hydro-300'
                  : 'text-gray-600 hover:text-gray-900 dark:text-gray-300 dark:hover:text-white'
              ]"
            >
              {{ link.name }}
            </router-link>

            <!-- Connection status -->
            <div class="flex items-center ml-4 pl-4 border-l border-gray-200 dark:border-gray-700">
              <span
                class="status-dot mr-2"
                :class="deviceConnected ? 'status-online' : 'status-offline'"
              ></span>
              <span class="text-sm text-gray-500 dark:text-gray-400">
                {{ deviceConnected ? 'Online' : 'Offline' }}
              </span>
            </div>
          </div>
        </div>
      </div>
    </nav>

    <!-- Main content -->
    <main class="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 py-8">
      <router-view :device-status="deviceStatus" @refresh="fetchStatus" />
    </main>

    <!-- Toast notifications -->
    <div class="fixed bottom-4 right-4 z-50">
      <transition-group name="toast">
        <div
          v-for="toast in toasts"
          :key="toast.id"
          class="mb-2 px-4 py-3 rounded-lg shadow-lg text-white max-w-sm"
          :class="{
            'bg-green-500': toast.type === 'success',
            'bg-red-500': toast.type === 'error',
            'bg-blue-500': toast.type === 'info'
          }"
        >
          {{ toast.message }}
        </div>
      </transition-group>
    </div>
  </div>
</template>

<script>
import { ref, onMounted, onUnmounted } from 'vue'
import api from './api/client'

export default {
  name: 'App',
  setup() {
    const navLinks = [
      { path: '/', name: 'Dashboard' },
      { path: '/schedule', name: 'Schedule' },
      { path: '/history', name: 'History' }
    ]

    const deviceStatus = ref(null)
    const deviceConnected = ref(false)
    const toasts = ref([])
    let statusInterval = null

    const fetchStatus = async () => {
      try {
        const response = await api.getStatus()
        deviceStatus.value = response.data
        deviceConnected.value = response.data.connected
      } catch (error) {
        deviceConnected.value = false
        console.error('Failed to fetch status:', error)
      }
    }

    const showToast = (message, type = 'info') => {
      const id = Date.now()
      toasts.value.push({ id, message, type })
      setTimeout(() => {
        toasts.value = toasts.value.filter(t => t.id !== id)
      }, 3000)
    }

    onMounted(() => {
      fetchStatus()
      // Poll status every 30 seconds
      statusInterval = setInterval(fetchStatus, 30000)
    })

    onUnmounted(() => {
      if (statusInterval) clearInterval(statusInterval)
    })

    return {
      navLinks,
      deviceStatus,
      deviceConnected,
      toasts,
      fetchStatus,
      showToast
    }
  }
}
</script>

<style>
.toast-enter-active,
.toast-leave-active {
  transition: all 0.3s ease;
}

.toast-enter-from,
.toast-leave-to {
  opacity: 0;
  transform: translateX(100px);
}
</style>
