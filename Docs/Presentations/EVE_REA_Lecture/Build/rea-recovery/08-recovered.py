def GetContractProxySvc(self):
    if self.contractProxySvc == None:
        self.contractProxySvc = sm.ProxySvc('contractProxy')
    return self.contractProxySvc
