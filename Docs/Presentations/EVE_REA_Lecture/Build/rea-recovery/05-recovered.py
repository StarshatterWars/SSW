def job_cost(self, job):
    return job.prices.get(job.blueprint.blueprintTypeID, 0) * float(job.runs)
