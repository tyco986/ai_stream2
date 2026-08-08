export type LoginBody = {
  username: string
  password: string
  new_password?: string
}

export type LoginResult = {
  must_change_password?: boolean
}
