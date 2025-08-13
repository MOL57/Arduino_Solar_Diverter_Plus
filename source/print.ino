// AUXILIARY PRINTING FUNCTIONS

void printTimes(void)
{
  snprintf_P(buffer,199,PSTR("%s\t"
                            "loop_us:%lu \t+display_us:%lu \t"
                            "sampling_us:%lu \tcomputing_us:%lu \t"
                            "next_decide_s:%d \tnext_refresh_s:%d"), 
                              CT.hhmmss, 
                              CT.loopTime_us, DS.displayTimeUs,
                              CV.samplingTimeAvg_us, CV.endUs - CV.startUs,
                              CT.countDecide_s, CT.countRefresh_s );
  
  Serial.println(buffer);
}

void printOneCycle()  //imprimeix els valors ADC mostrejats en un cicle de xarxa, permet utilitzar el Serial Plotter del IDE d'Arduino
{
  for(int i=0;i<SAMPLES_PER_CYCLE;i++)
  {
    sprintf_P(buffer,PSTR("i:%d \tsampling_us:%d\t\tV0:%d \tVx:%d \tIg:%d \tIc:%d"),i,CM.samplingUs[i],CM.V0[i],CM.Vx[i],CM.Ig[i],CM.Ic[i]);
    Serial.println(buffer);
  }
}

void printValues()  //Imprimeix els valors eficaços, potències i factors de potència calculats per al darrer cicle mostrejat
{
  snprintf_P(buffer, 249,   PSTR( "%s \tV0Avg:%ld \tuV/count:%ld \tVxEff_V:%d \t"
                                "IgEff_A:%d.%1d \tIcEff_A:%d.%1d \t"
                                "Pg_W:%d \tPc_W:%d \tPn_W:%d \tPFg:%d \tPFc:%d"),
                                CT.hhmmss, (long) round(CV.V0Avg), (long) round(10e6*CV.VoltsPerCount), (int)  round(CV.VxEff), 
                                 (int) (CV.IgEff), ( (int) (10.0*CV.IgEff) )%10, (int) (CV.IcEff), ( (int) (10.0*CV.IcEff) )%10,                               
                                 (int) round(CV.Pg), (int)  round(CV.Pc), (int) round(CV.Pn), (int) round(100.0*CV.PFg),  (int) round(100.0*CV.PFc) ); 
  Serial.println(buffer);     
}

void printFilteredValues() //imprimeix l'interval de mostreig en ms i els valors filtrats de potència generado, consumida i excedentària
{
  sprintf_P(buffer,PSTR("%s \tPgFilt_W:%d \tPcFilt_W:%d \tPnFilt_W:%d \tmargin_W:%d \tmax_consumption:%d"), 
                        CT.hhmmss, (int) round(CV.PgFilt), (int) round(CV.PcFilt), (int) round(CV.PnFilt), 
                        (int) round(CV.Margin), (int) round(CV.MaxConsumpt) );
  Serial.println(buffer);
}

