// Frozen installed0.2.67 pickup compositor. Only the symbol and out-of-line attribute differ.
// Original ui.cpp SHA256: e37bcec2df299f5db8cdf6766b07619d1666a88c97d32d0112a63e1e959bb143
__attribute__((noinline,unused)) void drawPickupFlightReference67(unsigned local,float sx,float sy,float tx=148,float ty=7){
 const int oldClip=dotClipBottom;dotClipBottom=122;
 // The source flares while its spirit flies out. A single translated frontal
 // stamp loses both this lingering flare and the changing flight direction.
 const unsigned sourceAlpha=collection_pickup::sourceOpacity(local);
 if(sourceAlpha)turnedSprite(winged_phoenix::pickup(),sx,sy,34.f+2.f*std::sin(local*.03f),24,0,sourceAlpha);
 CollectPoint prior[2]{};bool hasPrior=false;
 for(unsigned j=0;j<=64;++j){
  const int history=int(local)-int(j*collection_pickup::trailMs/64);
  if(history<0||history>int(collection_pickup::flightMs)){hasPrior=false;continue;}
  const auto p=pickupFlight(unsigned(history),sx,sy,tx,ty);
  const auto direction=pickupDirection(unsigned(history),sx,sy,tx,ty);
  const float dx=direction.x,dy=direction.y;
  const float length=std::max(.01f,std::sqrt(dx*dx+dy*dy));
  const float tailX=(winged_phoenix::flightTailX-winged_phoenix::flightAnchorX)*winged_phoenix::flightSpriteSize/32;
  const float tailY=(winged_phoenix::flightTailY-winged_phoenix::flightAnchorY)*winged_phoenix::flightSpriteSize/32;
  const float forwardX=std::cos(winged_phoenix::flightForwardAngle),forwardY=std::sin(winged_phoenix::flightForwardAngle);
  const float along=tailX*forwardX+tailY*forwardY,across=-tailX*forwardY+tailY*forwardX;
  const CollectPoint tail={p.x+(dx*along-dy*across)/length,p.y+(dy*along+dx*across)/length};
  const float gap=.8f+.3f*std::sin(history*.021f);
  const unsigned a=(230u*(64-j)*(64-j))/(64*64);
  for(unsigned lane=0;lane<2;++lane){
   const float side=lane?gap:-gap;const CollectPoint edge={tail.x-dy/length*side,tail.y+dx/length*side};
   if(hasPrior){
    for(int glow=-1;glow<=1;++glow)line(int(std::lround(prior[lane].x)),int(std::lround(prior[lane].y))+glow,int(std::lround(edge.x)),int(std::lround(edge.y))+glow,0xfba2,a/3);
    line(int(std::lround(prior[lane].x)),int(std::lround(prior[lane].y)),int(std::lround(edge.x)),int(std::lround(edge.y)),0xffe9,a);
   }
   prior[lane]=edge;
  }
  hasPrior=true;
 }
 const unsigned headAlpha=collection_pickup::headOpacity(local);
 if(headAlpha){
  const auto p=pickupFlight(local,sx,sy,tx,ty),direction=pickupDirection(local,sx,sy,tx,ty);
  const float angle=std::atan2(direction.y,direction.x)-winged_phoenix::flightForwardAngle;
  // The observed route follows the bird's visible body mass. Eye/beak
  // landmarks register the keys, but are not the path's body pivot.
  turnedSprite(winged_phoenix::flight(local),p.x,p.y,winged_phoenix::flightSpriteSize,winged_phoenix::flightSpriteSize,angle,headAlpha,winged_phoenix::flightAnchorX,winged_phoenix::flightAnchorY);
 }
 dotClipBottom=oldClip;
}
