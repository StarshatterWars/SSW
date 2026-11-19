def is_expired(self):
    if not self.expiration_time:
        return False
    return self.expiration_time - blue.os.GetWallclockTime() < 0
