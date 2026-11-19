def IsControlledByActiveShip(self):
    return self.GetController() == self.session.shipid
