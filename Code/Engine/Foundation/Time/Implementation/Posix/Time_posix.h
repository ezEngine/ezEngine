

void ezSystemTime::Initialize()
{
}

void ezSystemTime::Shutdown()
{
}

ezTime ezSystemTime::Now()
{
  timeval CurrentTime;
  gettimeofday(&CurrentTime, NULL);
  
  return ezTime((double)CurrentTime.tv_sec) + ezTime(ezTime::MicroSeconds(CurrentTime.tv_usec));
}
