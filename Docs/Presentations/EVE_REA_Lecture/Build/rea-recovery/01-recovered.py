def is_quantity_known(self, asteroid_id):
    return self.get_quantity(asteroid_id) not in (None, QUANTITY_UNKNOWN)
