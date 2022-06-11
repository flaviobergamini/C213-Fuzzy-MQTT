#include <Fuzzy.h>    // instalar a biblioteca eFLL
#include <math.h>
float PV=0; // inicializando o nível com zero
float Erro;
float DErro; 
float PVanterior;
float Saida=0;
int setpoint=40;
int i=0;
 
Fuzzy *fuzzy = new Fuzzy();
       // CONJUNTOS DA VARIÁVEL ERRO
       FuzzySet *MN = new FuzzySet(-200, -200, -4, -2);
       FuzzySet *PN = new FuzzySet(-4, -2, -2, 0);
       FuzzySet *ZE = new FuzzySet(-2, 0, 0, 2);
       FuzzySet *PP = new FuzzySet(0,2,2,4);
       FuzzySet *MP = new FuzzySet(2, 4, 200, 200);
       
       // CONJUNTOS DA VARIÁVEL DELTA ERRO
       FuzzySet *MNd = new FuzzySet(-200, -200, -4, -2);
       FuzzySet *PNd = new FuzzySet(-4, -2, -2, 0);
       FuzzySet *ZEd = new FuzzySet(-2, 0, 0, 2);
       FuzzySet *PPd = new FuzzySet(0,2,2,4);
       FuzzySet *MPd = new FuzzySet(2, 4, 200, 200);
       
       // CONJUNTOS DA VARIÁVEL POTÊNCIA DA BOMBA
       FuzzySet *MB = new FuzzySet(0,0,0,25);
       FuzzySet *B = new FuzzySet(0, 25, 25, 50);
       FuzzySet *M = new FuzzySet(25, 50, 50, 75);
       FuzzySet *A = new FuzzySet(50, 75, 75, 100);
       FuzzySet *MA = new FuzzySet(75, 100, 100, 100);


void setup()
{
  Serial.begin(115200);
  //  VARIAVEL erro
        FuzzyInput *Erro = new FuzzyInput(1);
        Erro->addFuzzySet(MN);
        Erro->addFuzzySet(PN);
        Erro->addFuzzySet(ZE);
        Erro->addFuzzySet(PP);
        Erro->addFuzzySet(MP);  
        fuzzy->addFuzzyInput(Erro);

  //  VARIAVEL delta erro
        FuzzyInput *DErro = new FuzzyInput(2);
        DErro->addFuzzySet(MNd);
        DErro->addFuzzySet(PNd);
        DErro->addFuzzySet(ZEd);
        DErro->addFuzzySet(PPd);
        DErro->addFuzzySet(MPd);  
        fuzzy->addFuzzyInput(DErro);
  
  //  VARIAVEL saida
        FuzzyOutput *Saida = new FuzzyOutput(1);
        Saida->addFuzzySet(MB);
        Saida->addFuzzySet(B);
        Saida->addFuzzySet(M);
        Saida->addFuzzySet(A);
        Saida->addFuzzySet(MA);  
        fuzzy->addFuzzyOutput(Saida);

  FuzzyRuleConsequent* thenSaidaMB = new FuzzyRuleConsequent();
  FuzzyRuleConsequent* thenSaidaB = new FuzzyRuleConsequent();
  FuzzyRuleConsequent* thenSaidaM = new FuzzyRuleConsequent();
  FuzzyRuleConsequent* thenSaidaA = new FuzzyRuleConsequent();
  FuzzyRuleConsequent* thenSaidaMA = new FuzzyRuleConsequent();

 
  // Building FuzzyRule "IF Erro=MN and DEerro = MN THEN saida = M"
  FuzzyRuleAntecedent* ifErroMNAndDErroMNd = new FuzzyRuleAntecedent();
  ifErroMNAndDErroMNd->joinWithAND(MN,MNd);
  thenSaidaM->addOutput(M);
  FuzzyRule *fuzzyRule01 = new FuzzyRule(1, ifErroMNAndDErroMNd, thenSaidaM);
  fuzzy->addFuzzyRule(fuzzyRule01);

  // Building FuzzyRule "IF Erro=PN and DEerro = MN THEN saida = M"
  FuzzyRuleAntecedent* ifErroPNAndDErroMNd = new FuzzyRuleAntecedent();
  ifErroPNAndDErroMNd->joinWithAND(PN,MNd);
  thenSaidaM->addOutput(M);
  FuzzyRule *fuzzyRule02 = new FuzzyRule(2, ifErroPNAndDErroMNd, thenSaidaM);
  fuzzy->addFuzzyRule(fuzzyRule02);

   // Building FuzzyRule "IF Erro=ZE and DEerro = MN THEN saida = M"
  FuzzyRuleAntecedent* ifErroZEAndDErroMNd = new FuzzyRuleAntecedent();
  ifErroZEAndDErroMNd->joinWithAND(ZE,MNd);
  thenSaidaM->addOutput(M);
  FuzzyRule *fuzzyRule03 = new FuzzyRule(3, ifErroZEAndDErroMNd, thenSaidaM);
  fuzzy->addFuzzyRule(fuzzyRule03);
  
   // Building FuzzyRule "IF Erro=PP and DEerro = MN THEN saida = M"
  FuzzyRuleAntecedent* ifErroPPAndDErroMNd = new FuzzyRuleAntecedent();
  ifErroPPAndDErroMNd->joinWithAND(PP,MNd);
  thenSaidaM->addOutput(M);
  FuzzyRule *fuzzyRule04 = new FuzzyRule(4, ifErroPPAndDErroMNd, thenSaidaM);
  fuzzy->addFuzzyRule(fuzzyRule04);

   // Building FuzzyRule "IF Erro=MP and DEerro = MN THEN saida = M"
  FuzzyRuleAntecedent* ifErroMPAndDErroMNd = new FuzzyRuleAntecedent();
  ifErroMPAndDErroMNd->joinWithAND(MP,MNd);
  thenSaidaM->addOutput(M);
  FuzzyRule *fuzzyRule05 = new FuzzyRule(5, ifErroMPAndDErroMNd, thenSaidaM);
  fuzzy->addFuzzyRule(fuzzyRule05);
 
    // Building FuzzyRule "IF Erro=MN and DEerro = PN THEN saida = M"
  FuzzyRuleAntecedent* ifErroMNAndDErroPNd = new FuzzyRuleAntecedent();
  ifErroMNAndDErroPNd->joinWithAND(MN,PNd);
  thenSaidaM->addOutput(M);
  FuzzyRule *fuzzyRule06 = new FuzzyRule(6, ifErroMNAndDErroPNd, thenSaidaM);
  fuzzy->addFuzzyRule(fuzzyRule06);

  // Building FuzzyRule "IF Erro=PN and DEerro = PN THEN saida = M"
  FuzzyRuleAntecedent* ifErroPNAndDErroPNd = new FuzzyRuleAntecedent();
  ifErroPNAndDErroPNd->joinWithAND(PN,PNd);
  thenSaidaM->addOutput(M);
  FuzzyRule *fuzzyRule07 = new FuzzyRule(7, ifErroPNAndDErroPNd, thenSaidaM);
  fuzzy->addFuzzyRule(fuzzyRule07);

   // Building FuzzyRule "IF Erro=ZE and DEerro = PN THEN saida = M"
  FuzzyRuleAntecedent* ifErroZEAndDErroPNd = new FuzzyRuleAntecedent();
  ifErroZEAndDErroPNd->joinWithAND(ZE,PNd);
  thenSaidaM->addOutput(M);
  FuzzyRule *fuzzyRule08 = new FuzzyRule(8, ifErroZEAndDErroPNd, thenSaidaM);
  fuzzy->addFuzzyRule(fuzzyRule08);
 
   // Building FuzzyRule "IF Erro=PP and DEerro = PN THEN saida = M"
  FuzzyRuleAntecedent* ifErroPPAndDErroPNd = new FuzzyRuleAntecedent();
  ifErroPPAndDErroPNd->joinWithAND(PP,PNd);
  thenSaidaM->addOutput(M);
  FuzzyRule *fuzzyRule09 = new FuzzyRule(9, ifErroPPAndDErroPNd, thenSaidaM);
  fuzzy->addFuzzyRule(fuzzyRule09);

   // Building FuzzyRule "IF Erro=MP and DEerro = MN THEN saida = M"
  FuzzyRuleAntecedent* ifErroMPAndDErroPNd = new FuzzyRuleAntecedent();
  ifErroMPAndDErroPNd->joinWithAND(MP,PNd);
  thenSaidaM->addOutput(M);
  FuzzyRule *fuzzyRule10 = new FuzzyRule(10, ifErroMPAndDErroPNd, thenSaidaM);
  fuzzy->addFuzzyRule(fuzzyRule10);
 
     // Building FuzzyRule "IF Erro=MN and DEerro = ZE THEN saida = M"
  FuzzyRuleAntecedent* ifErroMNAndDErroZEd = new FuzzyRuleAntecedent();
  ifErroMNAndDErroZEd->joinWithAND(MN,ZEd);
  thenSaidaM->addOutput(M);
  FuzzyRule *fuzzyRule11 = new FuzzyRule(11, ifErroMNAndDErroZEd, thenSaidaM);
  fuzzy->addFuzzyRule(fuzzyRule11);

  // Building FuzzyRule "IF Erro=PN and DEerro = ZE THEN saida = M"
  FuzzyRuleAntecedent* ifErroPNAndDErroZEd = new FuzzyRuleAntecedent();
  ifErroPNAndDErroZEd->joinWithAND(PN,ZEd);
  thenSaidaM->addOutput(M);
  FuzzyRule *fuzzyRule12 = new FuzzyRule(12, ifErroPNAndDErroZEd, thenSaidaM);
  fuzzy->addFuzzyRule(fuzzyRule12);

   // Building FuzzyRule "IF Erro=ZE and DEerro = ZE THEN saida = M"
  FuzzyRuleAntecedent* ifErroZEAndDErroZEd = new FuzzyRuleAntecedent();
  ifErroZEAndDErroZEd->joinWithAND(ZE,ZEd);
  thenSaidaM->addOutput(M);
  FuzzyRule *fuzzyRule13 = new FuzzyRule(13, ifErroZEAndDErroZEd, thenSaidaM);
  fuzzy->addFuzzyRule(fuzzyRule13);

   // Building FuzzyRule "IF Erro=PP and DEerro = ZE THEN saida = M"
  FuzzyRuleAntecedent* ifErroPPAndDErroZEd = new FuzzyRuleAntecedent();
  ifErroPPAndDErroZEd->joinWithAND(PP,ZEd);
  thenSaidaM->addOutput(M);
  FuzzyRule *fuzzyRule14 = new FuzzyRule(14, ifErroPPAndDErroZEd, thenSaidaM);
  fuzzy->addFuzzyRule(fuzzyRule14);

   // Building FuzzyRule "IF Erro=MP and DEerro = ZE THEN saida = M"
  FuzzyRuleAntecedent* ifErroMPAndDErroZEd = new FuzzyRuleAntecedent();
  ifErroMPAndDErroZEd->joinWithAND(MP,ZEd);
  thenSaidaM->addOutput(M);
  FuzzyRule *fuzzyRule15 = new FuzzyRule(15, ifErroMPAndDErroZEd, thenSaidaM);
  fuzzy->addFuzzyRule(fuzzyRule15);

  // Building FuzzyRule "IF Erro=MN and DEerro = PP THEN saida = M"
  FuzzyRuleAntecedent* ifErroMNAndDErroPPd = new FuzzyRuleAntecedent();
  ifErroMNAndDErroPPd->joinWithAND(MN,PPd);
  thenSaidaM->addOutput(M);
  FuzzyRule *fuzzyRule16 = new FuzzyRule(16, ifErroMNAndDErroPPd, thenSaidaM);
  fuzzy->addFuzzyRule(fuzzyRule16);

  // Building FuzzyRule "IF Erro=PN and DEerro = PP THEN saida = M"
  FuzzyRuleAntecedent* ifErroPNAndDErroPPd = new FuzzyRuleAntecedent();
  ifErroPNAndDErroPPd->joinWithAND(PN,PPd);
  thenSaidaM->addOutput(M);
  FuzzyRule *fuzzyRule17 = new FuzzyRule(17, ifErroPNAndDErroPPd, thenSaidaM);
  fuzzy->addFuzzyRule(fuzzyRule17);

   // Building FuzzyRule "IF Erro=ZE and DEerro = PP THEN saida = M"
  FuzzyRuleAntecedent* ifErroZEAndDErroPPd = new FuzzyRuleAntecedent();
  ifErroZEAndDErroPPd->joinWithAND(ZE,PPd);
  thenSaidaM->addOutput(M);
  FuzzyRule *fuzzyRule18 = new FuzzyRule(18, ifErroZEAndDErroPPd, thenSaidaM);
  fuzzy->addFuzzyRule(fuzzyRule18);
  
   // Building FuzzyRule "IF Erro=PP and DEerro = PP THEN saida = M"
  FuzzyRuleAntecedent* ifErroPPAndDErroPPd = new FuzzyRuleAntecedent();
  ifErroPPAndDErroPPd->joinWithAND(PP,PPd);
  thenSaidaM->addOutput(M);
  FuzzyRule *fuzzyRule19 = new FuzzyRule(19, ifErroPPAndDErroPPd, thenSaidaM);
  fuzzy->addFuzzyRule(fuzzyRule19);
  
   // Building FuzzyRule "IF Erro=MP and DEerro = PP THEN saida = M"
  FuzzyRuleAntecedent* ifErroMPAndDErroPPd = new FuzzyRuleAntecedent();
  ifErroMPAndDErroPPd->joinWithAND(MP,PPd);
  thenSaidaM->addOutput(M);
  FuzzyRule *fuzzyRule20 = new FuzzyRule(20, ifErroMPAndDErroPPd, thenSaidaM);
  fuzzy->addFuzzyRule(fuzzyRule20);
  
  // Building FuzzyRule "IF Erro=MN and DEerro = MP THEN saida = M"
  FuzzyRuleAntecedent* ifErroMNAndDErroMPd = new FuzzyRuleAntecedent();
  ifErroMNAndDErroMPd->joinWithAND(MN,MPd);
  thenSaidaM->addOutput(M);
  FuzzyRule *fuzzyRule21 = new FuzzyRule(21, ifErroMNAndDErroMPd, thenSaidaM);
  fuzzy->addFuzzyRule(fuzzyRule21);

  // Building FuzzyRule "IF Erro=PN and DEerro = MP THEN saida = M"
  FuzzyRuleAntecedent* ifErroPNAndDErroMPd = new FuzzyRuleAntecedent();
  ifErroPNAndDErroMPd->joinWithAND(PN,MPd);
  thenSaidaM->addOutput(M);
  FuzzyRule *fuzzyRule22 = new FuzzyRule(22, ifErroPNAndDErroMPd, thenSaidaM);
  fuzzy->addFuzzyRule(fuzzyRule22);

   // Building FuzzyRule "IF Erro=ZE and DEerro = MP THEN saida = M"
  FuzzyRuleAntecedent* ifErroZEAndDErroMPd = new FuzzyRuleAntecedent();
  ifErroZEAndDErroMPd->joinWithAND(ZE,MPd);
  thenSaidaM->addOutput(M);
  FuzzyRule *fuzzyRule23 = new FuzzyRule(23, ifErroZEAndDErroMPd, thenSaidaM);
  fuzzy->addFuzzyRule(fuzzyRule23);
  
   // Building FuzzyRule "IF Erro=PP and DEerro = MP THEN saida = M"
  FuzzyRuleAntecedent* ifErroPPAndDErroMPd = new FuzzyRuleAntecedent();
  ifErroPPAndDErroMPd->joinWithAND(PP,MPd);
  thenSaidaM->addOutput(M);
  FuzzyRule *fuzzyRule24 = new FuzzyRule(24, ifErroPPAndDErroMPd, thenSaidaM);
  fuzzy->addFuzzyRule(fuzzyRule24);
  
   // Building FuzzyRule "IF  Erro=MP and DEerro = MP THEN saida = M"
  FuzzyRuleAntecedent* ifErroMPAndDErroMPd = new FuzzyRuleAntecedent();
  ifErroMPAndDErroMPd->joinWithAND(MP,MPd);
  thenSaidaM->addOutput(M);
  FuzzyRule *fuzzyRule25 = new FuzzyRule(25, ifErroMPAndDErroMPd, thenSaidaM);
  fuzzy->addFuzzyRule(fuzzyRule25);
}


void loop()
{    
  Erro=PV-setpoint;
  fuzzy->setInput(1, Erro);
  fuzzy->setInput(2, DErro);
  fuzzy->fuzzify();
  Saida = fuzzy->defuzzify(1);
  Serial.println (String(PV)+";"+String(Erro)+";"+String(Saida));
  PVanterior=PV;
  PV=0.9954*PV+0.002763*Saida;
  DErro=PV-PVanterior;
  i=i+1;
  delay (300);
}
