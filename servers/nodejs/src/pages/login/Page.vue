<template>
  <div class="login-page">
    <form class="login-card" @submit.prevent="onSubmit">
      <h1 class="login-card__title">{{ t('login.title') }}</h1>
      <label class="login-card__field">
        <span>{{ t('login.username') }}</span>
        <UiInput
          :model-value="username"
          autocomplete="username"
          :disabled="mustChangePassword && !ticketMode"
          @update:model-value="username = $event"
        />
      </label>
      <label class="login-card__field">
        <span>{{ passwordLabel }}</span>
        <UiInput
          type="password"
          :model-value="password"
          autocomplete="current-password"
          :disabled="mustChangePassword && !ticketMode"
          @update:model-value="onPasswordInput"
        />
      </label>
      <label v-if="showNewPasswordFields" class="login-card__field">
        <span>{{ t('login.newPassword') }}</span>
        <UiInput
          type="password"
          :model-value="newPassword"
          autocomplete="new-password"
          @update:model-value="newPassword = $event"
        />
      </label>
      <label v-if="mustChangePassword && !ticketMode" class="login-card__field">
        <span>{{ t('login.confirmPassword') }}</span>
        <UiInput
          type="password"
          :model-value="confirmPassword"
          autocomplete="new-password"
          @update:model-value="confirmPassword = $event"
        />
      </label>
      <div class="login-card__actions">
        <UiButton type="primary" :loading="submitting" :disabled="submitting" @click="onSubmit">
          {{ submitLabel }}
        </UiButton>
      </div>
    </form>
  </div>
</template>

<script setup lang="ts">
import { computed, onMounted, ref } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { useI18n } from 'vue-i18n'
import { login } from '@/api/login'
import { getMe } from '@/api/shell'
import { ApiError } from '@/shared/http/client'
import UiButton from '@/shared/ui/Button.vue'
import UiInput from '@/shared/ui/Input.vue'
import { toast } from '@/shared/ui/toast'

const { t } = useI18n()
const router = useRouter()
const route = useRoute()

const username = ref('')
const password = ref('')
const newPassword = ref('')
const confirmPassword = ref('')
const mustChangePassword = ref(false)
const submitting = ref(false)

const ticketMode = computed(() => isJwtLike(password.value))

const showNewPasswordFields = computed(
  () => mustChangePassword.value || ticketMode.value,
)

const passwordLabel = computed(() => {
  let label = t('login.password')
  if (ticketMode.value) {
    label = t('login.ticketOrPassword')
  }
  return label
})

const submitLabel = computed(() => {
  let label = t('login.submit')
  if (mustChangePassword.value && !ticketMode.value) {
    label = t('login.changePasswordSubmit')
  }
  return label
})

function isJwtLike(value: string): boolean {
  const parts = value.split('.')
  return parts.length === 3 && Boolean(parts[0] && parts[1] && parts[2])
}

function redirectTarget(): string {
  const redirect = route.query.redirect
  if (typeof redirect === 'string' && redirect.startsWith('/')) {
    return redirect
  }
  return '/'
}

function onPasswordInput(value: string) {
  password.value = value
  if (isJwtLike(value)) {
    mustChangePassword.value = false
    confirmPassword.value = ''
  }
}

onMounted(async () => {
  try {
    await getMe()
    await router.replace(redirectTarget())
  } catch {
    // stay on login
  }
})

async function onSubmit() {
  if (submitting.value) {
    return
  }
  if (!username.value.trim()) {
    toast.error(t('login.usernameRequired'))
    return
  }
  if (!password.value) {
    toast.error(t('login.passwordRequired'))
    return
  }
  if (mustChangePassword.value && !ticketMode.value) {
    if (!newPassword.value) {
      toast.error(t('login.newPasswordRequired'))
      return
    }
    if (newPassword.value !== confirmPassword.value) {
      toast.error(t('login.passwordMismatch'))
      return
    }
  }
  submitting.value = true
  try {
    const body: {
      username: string
      password: string
      new_password?: string
    } = {
      username: username.value,
      password: password.value,
    }
    if (mustChangePassword.value && !ticketMode.value) {
      body.new_password = newPassword.value
    } else if (ticketMode.value && newPassword.value) {
      body.new_password = newPassword.value
    }
    const result = await login(body)
    if (result.must_change_password) {
      mustChangePassword.value = true
      newPassword.value = ''
      confirmPassword.value = ''
      toast.info(t('login.mustChangePassword'))
      return
    }
    await router.replace(redirectTarget())
  } catch (err) {
    toast.error(err instanceof ApiError ? err.message : String(err))
  } finally {
    submitting.value = false
  }
}
</script>

<style scoped>
.login-page {
  min-height: 100vh;
  display: flex;
  align-content: center;
  justify-content: center;
  align-items: center;
  background: linear-gradient(160deg, #f5f7fa 0%, #e4e7ed 100%);
  padding: 24px;
}

.login-card {
  width: 100%;
  max-width: 380px;
  background: #fff;
  border-radius: 8px;
  padding: 32px 28px 28px;
  box-shadow: 0 8px 24px rgba(0, 0, 0, 0.06);
  display: flex;
  flex-direction: column;
  gap: 16px;
}

.login-card__title {
  margin: 0 0 8px;
  font-size: 22px;
  font-weight: 600;
  text-align: center;
  color: #303133;
}

.login-card__field {
  display: flex;
  flex-direction: column;
  gap: 6px;
  font-size: 13px;
  color: #606266;
}

.login-card__actions {
  display: flex;
  justify-content: flex-end;
  margin-top: 8px;
}
</style>
