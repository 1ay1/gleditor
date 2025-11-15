// 2025-3-14
// apollo

#define REND2 (iMouse.x>iResolution.x*.5)

#define PI 3.1415926535

#define rot(t) mat2(cos(t), -sin(t),sin(t),cos(t))

int obj;

vec2 hash2( float n ) { return fract(sin(vec2(n,n+1.0))*vec2(43758.5453123,22578.1459123)); }

float apollo(vec3 p)
{
        float j=1.,k,rxy;
        float r0 = length(p)-.6;
        for(int i=0;i<9;i++){
                p = 2.*clamp(p, -2., 2.)-p;// p = 2.*clamp(p, -1.1, 1.1)-p;
                k = max(1., .70968/dot(p,p));
                p *= k;
                j = j * k + 0.05;

        }
        rxy = length(p.xy);
        return max(r0, max(rxy - .92784, abs(rxy * p.z) / length(p)) / j - 1e-4);
}


float map(vec3 p)
{
        float t = iTime;
        vec2 m = iMouse.xy/iResolution.xy*6.28;//-3.14;
        p.xy *= rot(2.);
        p.xz *= rot(PI/3.+9.8+m.x);
        p.yz *= rot(.516 +8.4+m.y/2.);

        p.xy *= rot(iTime+2.);


        obj=1;

        return apollo(p);
}

float calcShadow( in vec3 ro, in vec3 rd, float k )
{// iq - https://www.shadertoy.com/view/ldScDh
    float res = 1.0;

    float t = 0.01;
    for( int i=0; i<(REND2?40:128); i++ )
    {
        vec3 pos = ro + t*rd;
        float h = map( pos );
        res = min( res, k*max(h,0.0)/t );
        if( res<0.0001 || pos.y>10.0) break;
        t += clamp(h,0.01,5.0);
    }

    return res;
}

float calcOcclusion( in vec3 pos, in vec3 nor, float ra )
{// iq - https://www.shadertoy.com/view/ldScDh
    float occ = 0.0;
    for( int i=min(0,iFrame); i<32; i++ )
    {
        float h = 0.01 + 4.0*pow(float(i)/31.0,2.0);
        vec2 an = hash2( ra + float(i)*13.1 )*vec2( 3.14159, 6.2831 );
        vec3 dir = vec3( sin(an.x)*sin(an.y), sin(an.x)*cos(an.y), cos(an.x) );
        dir *= sign( dot(dir,nor) );
        occ += clamp( 5.0*map( pos + h*dir )/h, -1.0, 1.0);
    }
    return clamp( occ/32.0, 0.0, 1.0 );
}



void mainImage(out vec4 O, vec2 v)
{
        O = vec4(0.5,.2,0,1).xxxx;
        vec2 R = iResolution.xy,
             u = .6 * (v+v+.1 - R) / R.y,      // 实际坐标
             m = 1. * (iMouse.xy*2. - R) / R.y;// 实际鼠标
        vec3 o = vec3(0, 2, -6),               // 眼睛坐标
             r = normalize(vec3(u, 64)),        // 射线
             e = vec3(0, 1e-5, 0),             // 微距
             p,n,                                // 法向量
             s = normalize(vec3(-1,2,-3));     // 太阳
        r.y-= .333;
        r=normalize(r);
        float d,t,f,g,c;
        for(int i;i<22256 && t < 15.;i++)
        {
                p = o + r * t;
                d = map(p);
                if(d<.0003)break;

                t += d * .29;
        }
        obj=0;
        if(d<.0003)
        {
                        O *= 0.;
                        float ao = calcOcclusion(p-r*d, s, 1.0);
                        float shd = calcShadow(p-r*d, s, 100.);
                        n = normalize(vec3(map(p+e.yxx),map(p+e),map(p+e.xxy))-d);
                        f = .5 + .5 * dot(n, s);
                        g = max(dot(n,s),0.);
                        c = 1. + pow(f, 200.)-f*.3; // 665.352.6.542.9958.8.63
                        if(obj==1){
                              O += g*c;//(g+.4)*c;
                              O = mix(O*vec4(3, 2, 1,1), vec4(.5,0,0,1).xxxx, 1.-1./max(1., t*t*.01));
                        }else{
                              O=g*vec4(1.1);
                        }
                        O *= shd;//(REND2?shd:shd*.4+.6);
                        O *= min(.2*exp(-29.*p.z), 1.5);
                        //O  *= ao;


                        O = mix(O, vec4(.5), smoothstep(5.,15., t));
                        O = tanh(O);
        }
}
