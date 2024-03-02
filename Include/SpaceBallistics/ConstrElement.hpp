// vim:ts=2:et
//===========================================================================//
//                             "ConstrElement.hpp":                          //
//                 Geometrical Objects as Construction Elements              //
//===========================================================================//
#pragma  once
#include "SpaceBallistics/Types.hpp"
#include <boost/container/static_vector.hpp>
#include <cmath>
#include <cassert>
#include <type_traits>
#include <initializer_list>
#include <iostream>

namespace SpaceBallistics
{
  //=========================================================================//
  // "ConstrElement": Base Class for Construction Elements:                  //
  //=========================================================================//
  // General characteristics of standard construction elements:
  // Currently, 2D Shells (Truncated Cones and Spherical Segments), 3D Volumes
  // (of the same shape as Shells) and Point Masses are supported (via the cor-
  // resp Derived Classes of "ConstrElement").  For each them,  we can compute
  // Centers of Masses (CoM) and Moments of Inertia (MoI) wrt OX, OY, OZ axes.
  // NB:
  // It is assumed that the co-ords system OXYZ is such that  OX  is  the LV's
  // principal axis of symmetry, with the positive direction pointing DOWNWARDS
  // (ie from the Nose to the Tail). This is important when it comes to the CoM
  // and MoIs of the contained Propellant: we assume that it is concentrated at
  // the bottom (Larger-X) of the "ConstrElement":
  //
  class ConstrElement
  {
  public:
    //=======================================================================//
    // Consts and Types:                                                     //
    //=======================================================================//
    // "UnKnownMass": Param to be used for "ConstElement"s when their mass is
    // not yet known (all real Masses are of course strictly positive):
    //
    constexpr static Mass UnKnownMass = 0.0_kg;

    // The following types might be useful for derived classes:
    using Len2 = decltype(Sqr     (1.0_m));    // Same as "Area"
    using Len3 = decltype(Cube    (1.0_m));    // Same as "Vol"
    using Len4 = decltype(Sqr(Sqr (1.0_m)));
    using Len5 = decltype(Sqr(Sqr (1.0_m)) * 1.0_m);
    using Len6 = decltype(Sqr(Cube(1.0_m)));

  private:
    //=======================================================================//
    // Data Flds:                                                            //
    //=======================================================================//
    // Over-All Dynamical Params of this "ConstrElement":
    //
    Len         m_CoM [3];  // (X,Y,Z) co-ords of the Center of Masses
    Mass        m_mass;     // Mass
    MoI         m_MoIs[3];  // Moments of Inertia wrt the OX, OY and OZ axes
    bool        m_isFinal;  // If not set, "m_mass", "m_MoIs" are not valid yet
                            //   (but the CoM still is!)
  protected:
    //=======================================================================//
    // "Init":                                                               //
    //=======================================================================//
    // For use by Derived Classes. It is similar to a Non-Default Ctor, but is
    // intended to be invoked AFTER the Derived flds have been initialised, so
    // implemented as a Static Method, not a Ctor.
    // XXX: It is Static becaise without that, it could not be called from me-
    // thods of derived classes on "non-this" objs:
    //
    constexpr static void Init
    (
      ConstrElement*  a_ce,
      Len  const      a_com [3],
      Mass            a_mass,
      MoI  const      a_mois[3],
      bool            a_is_final
    )
    {
      assert(a_ce != nullptr);
      a_ce->m_CoM [0]  = a_com [0];
      a_ce->m_CoM [1]  = a_com [1];
      a_ce->m_CoM [2]  = a_com [2];
      a_ce->m_mass     = a_mass;
      a_ce->m_MoIs[0]  = a_mois[0];
      a_ce->m_MoIs[1]  = a_mois[1];
      a_ce->m_MoIs[2]  = a_mois[2];
      a_ce->m_isFinal  = a_is_final;

      // NB: Even if "a_mass" is not final, it must be positive:
      assert( IsPos(a_ce->m_mass)    && !IsNeg(a_ce->m_MoIs[0]) &&
             !IsNeg(a_ce->m_MoIs[1]) && !IsNeg(a_ce->m_MoIs[2]));
    }

  public:
    //=======================================================================//
    // Default Ctor:                                                         //
    //=======================================================================//
    // NB:
    // (*) All numeric fields are initialised to 0s, not to NaNs, so the "empty"
    //     obj (constructed by the Default Ctor) can be used as an initial value
    //     for summation ("+", "+=", etc);
    // (*) IsFinal is set to "true", because the empty "ConstrElement" is typi-
    //     cally used as the base for "+" which requires Final components:
    //
    constexpr ConstrElement()
    : m_CoM       {0.0_m,    0.0_m,    0.0_m},
      m_mass      (0.0),
      m_MoIs      {MoI(0.0), MoI(0.0), MoI(0.0)},
      m_isFinal   (true)     // !!!
    {}

    //=======================================================================//
    // Non-Default Ctor:                                                     //
    //=======================================================================//
    constexpr ConstrElement
    (
      Len         a_com [3],
      Mass        a_mass,
      MoI         a_mois[3],
      bool        a_is_final
    )
    { Init(this, a_com, a_mass, a_mois, a_is_final); }

    // Copy Ctor, Assignment and Equality are auto-generated...

    //=======================================================================//
    // "GetMassScale":                                                       //
    //=======================================================================//
    // Given a list of "ConstrElement"s  (all of them must have Non-Final Mass-
    // es) and the real Total Mass of them,  returns  a  dimension-less  factor
    // which could be applied (in "ProRateMass") to each "ConstrELement" to set
    // its correct Final Mass. It assumes that the densities  (surface  or vol)
    // of all "ConstrElement"s in the list are the same, ie their relative mas-
    // ses remain unchanged after scaling:
    //
    constexpr static double GetMassScale
      (std::initializer_list<ConstrElement const*> a_ces, Mass a_total_mass)
    {
      assert(IsPos(a_total_mass));

      // All "a_ces" must NOT be Final yet. Calculate their nominal total mass:
      Mass nomTotal = 0.0_kg;
      for (ConstrElement const* ce: a_ces)
      {
        assert(ce != nullptr && !(ce->m_isFinal));
        nomTotal  += ce->m_mass;
      }
      // Determine the Scale Factor (exists iff nomTotal > 0):
      assert(IsPos(nomTotal));
      double scale = double(a_total_mass / nomTotal);

      assert(scale > 0.0);
      return scale;
    }

    //=======================================================================//
    // "ProRateMass":                                                        //
    //=======================================================================//
    // Returns a new object (of any type derived from "ConstrElement") which
    // differes from the original one  by the Mass and MoIs being multiplied
    // by the ScaleFactor. XXX: It is assumed that any other flds of "Derived"
    // are UNAFFECTED by this scaling:
    //
    template<typename Derived>
    constexpr static  Derived ProRateMass(Derived const& a_der, double a_scale)
    {
      assert(a_scale > 0.0);

      // Create a copy of "a_der" using the Copy Ctor of Derived (which must
      // indeed be derived from "ConstrElement"):
      Derived copy(a_der);

      // Cannot adjust the Mass once it has been finalised:
      assert(!copy.m_isFinal);

      // Set the Mass and adjust the MoIs; but the CoM is unchanged:
      copy.m_mass    *= a_scale;
      copy.m_MoIs[0] *= a_scale;
      copy.m_MoIs[1] *= a_scale;
      copy.m_MoIs[2] *= a_scale;
      copy.m_isFinal  = true;
      return copy;
    }

    //=======================================================================//
    // Accessors:                                                            //
    //=======================================================================//
    // Geometrical params are always available, even if the Mass is not final
    // yet:
    using     Point = decltype(m_CoM);
    using     MoIs  = decltype(m_MoIs);

    constexpr Point const& GetCoM() const { return m_CoM; }

    // But the Mass and MoIs  may or may not be available.
    // If not, assert failure will be signaled (NB: in C++ >= 14, "assert" is
    // allowed in "constexpr" functions, whereas throwing exceptions is not):
    //
    constexpr bool IsFinal() const { return m_isFinal; }

    constexpr Mass GetMass() const
    {
      assert(m_isFinal);
      return m_mass;
    }

    constexpr MoIs const& GetMoIs() const
    {
      assert(m_isFinal);
      return m_MoIs;
    }

    //=======================================================================//
    // Addition / Subtraction:                                               //
    //=======================================================================//
    // The summands must both have FINAL masses. Only one summand is allowed to
    // have zero mass, otherwise we cannot compute the CoM.  WE ASSUME that the
    // summands DO NOT INTERSECT in space:
    //
    constexpr ConstrElement& operator+= (ConstrElement const& a_right)
    {
      assert(m_isFinal && a_right.m_isFinal &&
             !(IsZero(m_mass) && IsZero(a_right.m_mass)));

      // Masses, SurfAreas, Vols and MoIs are directly-additive:
      Mass    m0  = m_mass;
      m_mass     += a_right.m_mass;
      m_MoIs[0]  += a_right.m_MoIs[0];
      m_MoIs[1]  += a_right.m_MoIs[1];
      m_MoIs[2]  += a_right.m_MoIs[2];

      // For the CoM, do the weighted avg (but the total mass must be non-0):
      assert(IsPos(m_mass));
      double mu0  = double(m0             / m_mass);
      double mu1  = double(a_right.m_mass / m_mass);
      m_CoM[0]    = mu0 * m_CoM[0]  + mu1 * a_right.m_CoM[0];
      m_CoM[1]    = mu0 * m_CoM[1]  + mu1 * a_right.m_CoM[1];
      m_CoM[2]    = mu0 * m_CoM[2]  + mu1 * a_right.m_CoM[2];
      return *this;
    }

    constexpr ConstrElement operator+ (ConstrElement const& a_right) const
    {
      ConstrElement res = *this;
      res += a_right;
      return res;
    }

    // Subtraction is also possible, but ONLY IF the resulting SurfArea, Vol
    // and Mass remain valid. USE THIS OP WITH CARE: It is intended for removal
    // of particular components from "ConstrElement" systems:
    //
    constexpr ConstrElement& operator-= (ConstrElement const& a_right)
    {
      assert(m_isFinal && a_right.m_isFinal &&
             !(IsZero(m_mass) && IsZero(a_right.m_mass)));

      // Masses, SurfAreas, Vols and MoIs are directly-additive/subtractable:
      Mass m0     = m_mass;
      m_mass     -= a_right.m_mass;
      m_MoIs[0]  -= a_right.m_MoIs[0];
      m_MoIs[1]  -= a_right.m_MoIs[1];
      m_MoIs[2]  -= a_right.m_MoIs[2];
      // CHECKS:
      assert(IsPos(m_mass)    && IsPos(m_MoIs[0]) &&
             IsPos(m_MoIs[1]) && IsPos(m_MoIs[2]));

      // For the CoM, do the weighted avg:
      double mu0  = double(m0             / m_mass);
      double mu1  = double(a_right.m_mass / m_mass);
      m_CoM[0]    = mu0 * m_CoM[0]  - mu1 * a_right.m_CoM[0];
      m_CoM[1]    = mu0 * m_CoM[1]  - mu1 * a_right.m_CoM[1];
      m_CoM[2]    = mu0 * m_CoM[2]  - mu1 * a_right.m_CoM[2];
      return *this;
    }

    constexpr ConstrElement operator- (ConstrElement const& a_right) const
    {
      ConstrElement res = *this;
      res -= a_right;
      return res;
    }
  };

  //=========================================================================//
  // "PointMass":                                                            //
  //=========================================================================//
  // A positive mass concentrated in the (x0, y0, z0) point:
  //
  class PointMass final: public ConstrElement
  {
  public:
    //-----------------------------------------------------------------------//
    // Non-Default Ctor:                                                     //
    //-----------------------------------------------------------------------//
    constexpr PointMass(Len a_x0, Len a_y0, Len a_z0, Mass a_mass)
    : ConstrElement()  // Slightly sub-optimal...
    {
      Len  pt[3] { a_x0, a_y0, a_z0 };

      // Compute the MoIs:
      Len2 x2 = Sqr(a_x0);
      Len2 y2 = Sqr(a_y0);
      Len2 z2 = Sqr(a_z0);
      assert(IsPos(a_mass));

      MoI  mois[3]
      {
        a_mass * (y2 + z2), // Distance^2 to the OX axis
        a_mass * (x2 + z2), // Distance^2 to the OY axis
        a_mass * (x2 + y2), // Distance^2 to the OZ axis
      };

      // Now the actual Base Class initialisation. NB: The point-mass is consi-
      // dered to be Final:
      ConstrElement::Init(this, pt, a_mass, mois, true);
    }
  };

  //=========================================================================//
  // "RotationBody":                                                         //
  //=========================================================================//
  // SubClass of "ConstrElement", SuperClass of "TrCone", "SpherSegment" and
  // others. Provides common functionality of  Rotation Surfaces and Bodies:
  //
  class RotationBody: public ConstrElement
  {
  protected:
    //=======================================================================//
    // Consts:                                                               //
    //=======================================================================//
    // Computation Tolerances:
    constexpr static double Tol     = 100.0 * Eps<double>;
    constexpr static double TolFact = 1.0   + Tol;

  private:
    //=======================================================================//
    // Data Flds:                                                            //
    //=======================================================================//
    // Orientation of the rotation axis: "alpha" (|alpha| < Pi/2)  is the angle
    // between the positive direction of the OX axis and the rotation axis. The
    // latter is assumed to lie in the OXY plane or in the OXZ plane  (or both,
    // in which case it coincides with OX);  MoIs are computed wrt (xR, yR, zR)
    // which is the "right" (Lower, Larger-X) rotation axis end;  we must have
    // yR=0 (the rotation axis is in OXY)  or zR=0 (in OXZ), or both (in which
    // case alpha=0 as well):
    //
    bool        m_inXY;         // If false, then inXZ holds (both may be true)
    bool        m_inXZ;         //
    double      m_cosA;         // cos(alpha)
    double      m_sinA;         // sin(alpha)
    Len         m_left [3];     // Left  (Upper, Smaller-X) axis end
    Len         m_h;            // Over-all body length along the rotation axis
    Len         m_right[3];     // Right (Lower, Larger-X) axis end: MoI ORIGIN
    Len         m_yzR;          // right[1] or right[2]

    // Geometric Properties:
    Area        m_sideSurfArea; // W/o the Bases
    Vol         m_enclVol;      // Nominal Volume enclosed (with imag. Bases)

    // Propellant-related flds (ie it is assumed that this rotation body may
    // contain Propellant):
    //
    Density     m_rho;          // Propellant Density (0 if no propellant)
    Mass        m_propMassCap;  // Propellant Mass Capacity

    // Coeffs for translation of "instrinsic" MoI params (J0, J1, K) into XYZ
    // MoI params (Jx, Jin, Jort and ultimately to Jx, Jy, Jz):
    // For Jx:
    double      m_Jx0;
    double      m_Jx1;
    Len         m_JxK;
    Len2        m_JxSV;
    // For Jin:
    double      m_Jin0;
    double      m_Jin1;
    Len         m_JinK;
    Len2        m_JinSV;
    // For Jort (Jort0 = Jort1 = 1):
    Len         m_JortK;
    Len2        m_JortSV;

    // Coeffs for computation of "intrinsic" MoI params (J0, J1, K) as polynom-
    // ial functions of the propellant level.  XXX: Interestingly, the degrees
    // of such polynomials are the same for the concrete hapes currently imple-
    // mented ("TrCone", "SpherSegm", ...), though extra coeffs may potentially
    // be required for other shapes:
    double      m_JP05;     // coeff(JP0, l^5)
    Len         m_JP04;     // coeff(JP0, l^4)
    Len2        m_JP03;     // coeff(JP0, l^3)
    double      m_JP15;     // coeff(JP1, l^5)
    Len         m_JP14;     // coeff(JP1, l^4)
    Len2        m_JP13;     // coeff(JP1, l^3)
    Len3        m_JP12;     // coeff(JP1, l^2)
    Len4        m_JP11;     // coeff(JP1, l)
    double      m_KP4;      // coeff(KP,  l^4)
    Len         m_KP3;      // coeff(KP,  l^3)
    Len2        m_KP2;      // coeff(KP,  l^2)

    // Emulating virtual methods:
    // The following function ptr is provided by derived classes:
    typedef    Len (*LevelOfVol) (Vol, RotationBody const*);
    LevelOfVol m_LoV;

  protected:
    // Default and Copy Ctors, Assignment and Equality are auto-generated, and
    // they are Protected:
    constexpr RotationBody()                                      = default;
    constexpr RotationBody            (RotationBody const&)       = default;
    constexpr RotationBody& operator= (RotationBody const&)       = default;
    constexpr bool          operator==(RotationBody const&) const = default;
    constexpr bool          operator!=(RotationBody const&) const = default;

    //=======================================================================//
    // "Init":                                                               //
    //=======================================================================//
    // Similar to "ConstrElement::Init": Initialiser similar to Non-Default Ctor
    // but used in a slightly different way:
    //
    constexpr void Init
    (
      // Params for the Base Class ("ConstrElement"):
      Area        a_side_surf_area,
      Vol         a_encl_vol,
      Mass        a_mass,     // If 0, then auto-calculated with SurfDens=1

      // Params for "RotationBody" itself:
      double      a_alpha,    // Rotation axis orientation
      Len         a_x0,       // The left or right axis end
      Len         a_y0,       //
      Len         a_z0,       //
      bool        a_0is_left, // (x0,y0,z0) is the left or right end?
      Len         a_h,        // Over-all body length  (along the rotation axis)

      // "Empty" MoI Coeffs (wrt the RIGHT end of the rotation axis):
      Len4        a_je0,
      Len4        a_je1,
      Len3        a_ke,

      // Propellant Density (may be 0):
      Density     a_rho,

      // Propellant Vol -> Propellant Level:
      LevelOfVol  a_lov,

      // Propellant MoI Coeffs as functions of Propellant Level (also wrt RIGHT
      // end of the rotation axis):
      double      a_jp05,
      Len         a_jp04,
      Len2        a_jp03,

      double      a_jp15,
      Len         a_jp14,
      Len2        a_jp13,
      Len3        a_jp12,
      Len4        a_jp11,

      double      a_kp4,
      Len         a_kp3,
      Len2        a_kp2
    )
    {
      //---------------------------------------------------------------------//
      // Initialise the "RotationBody" flds FIRST:                           //
      //---------------------------------------------------------------------//
      // Co-ords of the Right (Lower, Larger-X) base center (ie right end of
      // the rotation axis):
      m_inXY = IsZero(a_z0);
      m_inXZ = IsZero(a_y0);
      assert(  m_inXY || m_inXZ);
      assert(!(m_inXY && m_inXZ) || a_alpha == 0.0);

      // Rotation axis orientation angle, and the over-all length:
      m_cosA  = Cos(a_alpha);
      m_sinA  = Sin(a_alpha);
      m_h     = a_h;
      assert(m_cosA > 0.0 && IsPos(m_h));

      // Left and right ends of the rotation axis:
      if (a_0is_left)
      {
        m_left [0] = a_x0;
        m_left [1] = a_y0;
        m_left [2] = a_z0;
        Len dyz    =          m_sinA * m_h;
        m_right[0] = a_x0   + m_cosA * m_h;
        m_right[1] = m_inXY ? (a_y0  + dyz) : 0.0_m;
        m_right[2] = m_inXZ ? (a_z0  + dyz) : 0.0_m;
      }
      else
      {
        m_right[0] = a_x0;
        m_right[1] = a_y0;
        m_right[2] = a_z0;
        Len dyz    =          m_sinA * m_h;
        m_left [0] = a_x0   - m_cosA * m_h;
        m_left [1] = m_inXY ? (a_y0  - dyz) : 0.0_m;
        m_left [2] = m_inXZ ? (a_z0  - dyz) : 0.0_m;
      }
      m_yzR = m_inXY  ? m_right[1] : m_right[2];

      // Side Surace Area and Nominal Volume Enclosed:
      m_sideSurfArea = a_side_surf_area;
      assert(IsPos(m_sideSurfArea));

      m_enclVol      = a_encl_vol;
      assert(IsPos(m_enclVol));

      // Propellant Params:
      m_rho          = a_rho;
      m_propMassCap  = m_rho * a_encl_vol;
      m_LoV          = a_lov;

      // IMPORTANT!
      // Coeffs for (Jx, Jy, Jz) wrt (J0, J1, K, SurfOrVol), where J0, J1, K
      // are computed relative to the RIGHT end-point of the rotation axis.
      // This is just a matter of choice for 2D Shells, but becomes important
      // for 3D Propellant Volumes: as Propellants are spent, the Right end
      // of the Propellant Volume remains unchanged, whereas the Left one moves
      // with the decreasing level of Propellant:
      //
      // Jx =  m_sinA^2   * J0 + (1.0 + m_cosA^2)    * J1 +
      //       yzR * (yzR * SurfOrVol + 2.0 * m_sinA * K) :
      m_Jx0     = Sqr(m_sinA);
      m_Jx1     = 1.0 + Sqr(m_cosA);
      m_JxK     = 2.0 * m_sinA * m_yzR;
      m_JxSV    = Sqr(m_yzR);

      // Jin = m_cosA^2   * J0 + (1.0 + m_sinA^2)    * J1 +
      //       xR  * (xR  * SurfOrVol + 2.0 * m_cosA * K) :
      m_Jin0    = Sqr(m_cosA);
      m_Jin1    = 1.0 + Sqr(m_sinA);
      m_JinK    = 2.0 * m_cosA * m_right[0];
      m_JinSV   = Sqr(m_right[0]);

      // Jort = J0  + J1    + (Sqr(xR)    + Sqr(yzR)) * SurfOrVol +
      //        2.0 * (cosA * xR  + sinA * yzR) * K :
      m_JortK   = 2.0 * (m_cosA   * m_right[0] + m_sinA * m_yzR);
      m_JortSV  = Sqr(m_right[0]) + Sqr(m_yzR);

      // Coeffs for computing (J0, J1, K) as polynomial functions of the Propel-
      // lant Level:
      m_JP05 = a_jp05;
      m_JP04 = a_jp04;
      m_JP03 = a_jp03;

      m_JP15 = a_jp15;
      m_JP14 = a_jp14;
      m_JP13 = a_jp13;
      m_JP12 = a_jp12;
      m_JP11 = a_jp11;

      m_KP4  = a_kp4;
      m_KP3  = a_kp3;
      m_KP2  = a_kp2;

      //---------------------------------------------------------------------//
      // Checks:                                                             //
      //---------------------------------------------------------------------//
      // The rotation axis lies in the OXY plane, or OXZ plane, or in both:
      assert(m_inXY || m_inXZ);

      // If the rotation axis lies in both OXY and OXZ, then it must coincide
      // with OX, and therefore, we must have alpha=0:
      assert(!(m_inXY && m_inXZ) || (m_cosA == 1.0 && m_sinA == 0.0));

      // In general, |alpha| < Pi/2:
      assert(m_cosA > 0.0);

      // Propellant Density must be non-negative (0 is OK if no propellant):
      assert(!IsNeg(m_rho));

      // The LevelOfVolume function ptr must be non-NULL:
      assert(m_LoV != nullptr);

      //---------------------------------------------------------------------//
      // Now the Base Class ("ConstrElement"):                               //
      //---------------------------------------------------------------------//
      // The mass may or may not be given. If it is given,  calculate the Surf-
      // ace Density; otherwise, assume the SurfDens to be 1.0.   Then set the
      // Mass and MoIs:
      bool     isFinal  = IsPos(a_mass);
      SurfDens surfDens =
        isFinal ? (a_mass /  a_side_surf_area) : SurfDens(1.0);
      Mass emptyMass    =
        isFinal ?  a_mass : (a_side_surf_area  * surfDens);

      // "Empty" CoM and MoIs (2D) for the Base Class:
      Len emptyCoM [3];
      MoI emptyMoIs[3];
      MoIsCoM
        (a_je0, a_je1, a_ke, a_side_surf_area, surfDens, emptyCoM, emptyMoIs);

      // Check: "m_inXY", "m_inXZ" derived from "a_right" must be consistent
      // with "emptyCoM":
      assert(!m_inXY || IsZero(emptyCoM[2]));
      assert(!m_inXZ || IsZero(emptyCoM[1]));

      // Finally, invoke the Base Class Initialiser:
      ConstrElement::Init(this, emptyCoM, emptyMass, emptyMoIs, isFinal);
    }

    //=======================================================================//
    // "MoIsCoM": Computation of XYZ MoIs and CoM from "intrinsic" MoIs:     //
    //=======================================================================//
    // The same functions are applicable to both MoI per SurfDens (Len4) and
    // MoI per (Volume) Density (Len5), hence:
    // J=Len4, K=Len3, SV=Len2, OR
    // J=Len5, K=Len4, SV=Len3:
    //
    template<typename J>
    constexpr void MoIsCoM
    (
      J                          a_j0,      // Len4 or Len5, wrt RIGHT axis end
      J                          a_j1,      // ditto
      decltype(J(1.0)/1.0_m)     a_k,       // Len3 or Len4
      decltype(J(1.0)/Len2(1.0)) a_sv,      // Len2 or Len3 (ie SurfArea or Vol)
      decltype(1.0_kg/a_sv)      a_dens,    // SurfDens or Density
      Len                        a_com [3], // Output
      MoI                        a_mois[3]  // ditto
    )
    const
    {
      // "Intrinsic" MoI components must be > 0 for any finite body size.
      // But "a_k" must be negative (relative to the right-most point):
      assert(IsPos(a_j0) && IsPos(a_j1) && IsPos(a_sv) && IsNeg(a_k));

      // MoIs:
      J Jx   = m_Jx0  * a_j0 + m_Jx1  * a_j1 + m_JxK   * a_k + m_JxSV   * a_sv;
      J Jin  = m_Jin0 * a_j0 + m_Jin1 * a_j1 + m_JinK  * a_k + m_JinSV  * a_sv;
      J Jort =          a_j0 +          a_j1 + m_JortK * a_k + m_JortSV * a_sv;
      J Jy   = m_inXY ? Jin  : Jort;
      J Jz   = m_inXZ ? Jin  : Jort;

      a_mois[0]  = a_dens * Jx;
      a_mois[1]  = a_dens * Jy;
      a_mois[2]  = a_dens * Jz;
      assert(!(IsNeg(a_mois[0]) || IsNeg(a_mois[1]) || IsNeg(a_mois[2])));

      // Now the CoM: "xiC" is its co-ord along the rotation axis, relative to
      // the right-most axis point:
      Len  xiC   = a_k / a_sv;
      a_com[0]   = m_right[0]   + m_cosA * xiC;
      Len  yzC   = m_yzR        + m_sinA * xiC;
      a_com[1]   = m_inXY ? yzC : 0.0_m;
      a_com[2]   = m_inXZ ? yzC : 0.0_m;
    }

  public:
    //=======================================================================//
    // Elementary Accessors:                                                 //
    //=======================================================================//
    constexpr Area GetSideSurfArea() const { return m_sideSurfArea; }
    constexpr Vol  GetEnclVol()      const { return m_enclVol;      }
    constexpr Len  GetHeight ()      const { return m_h;            }

    //=======================================================================//
    // "GetPropCE":                                                          //
    //=======================================================================//
    // Constructs a "ConstrElement" which holds the CoM and MoI of the Propel-
    // lant filling this RotationBody, with the current propellant mass given
    // by "a_prop_mass".   The resulting "ConstrElement" does NOT include the
    // Shell!
    // Uses the "LevelOfVol" func installed by the derived class, in the NON-
    // virtual way.
    // Returns a ficticious "ConstrElement" obj acting as a container for the
    // results computed, and suitable for the "+" operation:
    // Although declared "constexpr", this function will mostly be called at
    // Run-Time. For the info, it also returns the PropLevel if the output ptr
    // is non-NULL:
    //
    constexpr ConstrElement   GetPropCE
      (Mass a_prop_mass, Len* a_prop_level = nullptr) const
    {
      // Check the limits of the Propellant Mass. For the 2nd inequaluty, allow
      // some floating-point tolerance:
      assert(!IsNeg(a_prop_mass) && a_prop_mass <= GetPropMassCap() * TolFact);

      // The Propellant Volume: Make sure it is within the limits to avoid
      // rounding errors:
      Vol propVol = a_prop_mass / GetPropDens();
      assert    (!IsNeg (propVol) && propVol <= GetEnclVol() * TolFact);
      propVol = std::min(propVol, GetEnclVol());

      // The Propellant Level, relative to the Right (Lower) Base.
      // XXX: We assume that the propellant surface is always orthogonal to the
      // rotation axis, due to the tank pressurisation:
      Len l = m_LoV(propVol, this);

      // Mathematically, we must have 0 <= l <= h, where l=0 corresponds to the
      // empty segment, and l=h to the full one. We enforce this interval expli-
      // citly to prevent rounding errors:
      assert(!IsNeg(l) && l <= m_h * TolFact);
      l        = std::min(l,   m_h);

      if (a_prop_level != nullptr)
        * a_prop_level  = l;

      // "Intrinsic" MoIs components of the Priopellant:
      Len2 l2  = Sqr (l);
      Len3 l3  = l2 * l;
      Len5 JP0 = (   (m_JP05 * l + m_JP04) * l + m_JP03) * l3;
      Len5 JP1 = (((((m_JP15 * l + m_JP14) * l + m_JP13) * l) + m_JP12)
                             * l + m_JP11) * l;
      Len4 KP  = (   (m_KP4  * l + m_KP3 ) * l + m_KP2 ) * l2;
      assert(!IsNeg(JP0) && !IsNeg(JP1) && !IsPos(KP));

      // Get the MoIs and the CoM of the Popellant (returned straight to the
      // CallER). NB: Needless to say, use the current "propVol" here, NOT
      // the maximum "GetEnclVol()":
      Len com [3];
      MoI mois[3];
      MoIsCoM(JP0, JP1, KP, propVol, GetPropDens(), com, mois);

      // Construct the result (XXX: this is slightly sub-optimal, as involves
      // copying of "com" and "mois"). NB:
      // (*) We must  install the correct Mass of the Propellant in the "res",
      //     so that  it can then participate in "operator+";
      // (*) SurfArea is not computed and remains 0;, we do not compute it;
      //     this is OK because by SurfArea we mean that of the Shell (with
      //     some non-0 SurfDens), whereas in this case, the Shell is not
      //     included in the result;
      // (*) the result is Final:
      //
      return ConstrElement(com, a_prop_mass, mois, true);
    }

    //=======================================================================//
    // Accessors (for use by Derived Classes):                               //
    //=======================================================================//
    constexpr Point const& GetLeft ()        const { return m_left;  }
    constexpr Point const& GetRight()        const { return m_right; }

    // The Maximum Propellant Mass (Capacity):
    constexpr Mass         GetPropMassCap()  const { return m_propMassCap; }

    // The Propellant Density:
    constexpr Density      GetPropDens()     const { return m_rho; }
  };

  //=========================================================================//
  // "TrCone": Truncated Cone Shell (w/o and with Propellant):               //
  //=========================================================================//
  // Object  : Truncated (in general) conical shell (side surface only), with
  //           SurfDensity assumed to be 1 (this may later be changed by cal-
  //           ling "ProRateMass" on the object constructed).
  // Geometry: Base diameters "d0" and "d1", height "h";  may have  d0<=>d1 ;
  //           either "d0" or "d1" may be be 0 (ie full cone), but  not both.
  // Location: XXX: Either "z0" or "y0" must be 0. The cone axis is lying in
  //           the XY or XZ plane, resp., at the angle  "alpha" to the posit-
  //           ive direction of the OX axis; we assume that |alpha| < Pi/2;
  //           the "d0" base diameter corresponds to the base center (x0,y0)
  //           (if z0=0, ie XY plane) or (x0,z0) (if y0=0, ie XZ plane); the
  //           "d1" diameter corresponds to  the other end of the cone axis
  //           segment (with X = x1 > x0);
  //           when alpha=0, the axis of the Spherical Segment coincides with
  //           OX, which is the most common case.
  // Mass:     If the "mass" param is set, its value is used  for the element
  //           mass which is in this case final. If this param is 0, the mass
  //           is set under the assumtion of SutfDensity=1, and it is NOT fi-
  //           nal yet (needs to be set later via "ProRateMass").
  // Return value: The resulting "ConstrElement" object.
  //
  class TrCone final: public RotationBody
  {
  private:
    //=======================================================================//
    // Data Flds: Truncated Cone's Geometry:                                 //
    //=======================================================================//
    // Over-all sizes:
    Len       m_r;   // Left  (Upper, Smaller-X) base radius
    Len       m_R;   // Right (Lower, Larger-X)  base radius
    Len       m_h;   // Duplicate of RotationBody::m_h which is "private"

    // Memoise pre-computed coeffs of the Cardano formula in "LevelOfVol":
    Len       m_deltaR;
    Len3      m_clVol;
    Len2      m_Rh;
    Len6      m_Rh3;

    // Default Ctor is deleted:
    TrCone() = delete;

    //=======================================================================//
    // Propellant Volume -> Propellant Level:                                //
    //=======================================================================//
    constexpr static Len LevelOfVol(Vol a_v, RotationBody const* a_ctx)
    {
      // "a_ctx" must actually be a ptr to "TrCone":
      TrCone const* trc = static_cast<TrCone const*>(a_ctx);
      assert(trc != nullptr);
      return trc->LevelOfVol(a_v);
    }

    constexpr Len LevelOfVol(Vol a_v) const
    {
      assert(!IsNeg(a_v) && a_v <= GetEnclVol());
      return
        IsZero(m_deltaR)
        ? // R==r: The simplest and the most common case: A Cylinder:
          double(a_v / GetEnclVol()) * m_h

        : // General case: Solving a cubic equation by the Cardano formula;
          // since Vol'(l) > 0 globally, there is only 1 real root:
          (m_Rh - CbRt(m_Rh3 - m_clVol * a_v)) / m_deltaR;
    }

  public:
    //=======================================================================//
    // Non-Default Ctor:                                                     //
    //=======================================================================//
    constexpr TrCone
    (
      Len         a_x0,       // Left  (upper) base center (Left rot.axis end)
      Len         a_y0,       //
      Len         a_z0,       //
      double      a_alpha,    // Dimension-less, in (-Pi/2 .. Pi/2)
      Len         a_d0,       // Left  (Upper, Smaller-X) base diameter
      Len         a_d1,       // Right (Lower, Larger-X)  base diameter
      Len         a_h,        // Height
      Density     a_rho,      // Propellant Density (may be 0)
      Mass        a_empty_mass = UnKnownMass
    )
    {
      //---------------------------------------------------------------------//
      // Over-All Geometry:                                                  //
      //---------------------------------------------------------------------//
      assert( IsPos(a_h)   && !IsNeg(a_d0)  && !IsNeg(a_d1) &&
            !(IsZero(a_d0) && IsZero(a_d1)) && Abs(a_alpha) < Pi_2<double> &&
            ! IsNeg(a_rho) && !IsNeg(a_empty_mass));

      // The over-all sizes:
      m_r        = a_d0 / 2.0;   // Left  (Upper, Smaller-X) base radius
      m_R        = a_d1 / 2.0;   // Right (Lower, Larger-X)  base radius
      m_h        = a_h;
      m_deltaR   = m_R - m_r;
      Len2  h2   = Sqr(m_h);

      // Memoised coeffs of the Cardano formula (used in "LevelOfVol"):
      m_clVol    = (3.0 / Pi<double>) * h2 * m_deltaR;
      m_Rh       = m_R * m_h;
      m_Rh3      = Cube (m_Rh);

      //---------------------------------------------------------------------//
      // Parent Classes Initialisation:                                      //
      //---------------------------------------------------------------------//
      // For Optimisation:
      Len    s    = SqRt(Sqr(m_deltaR) + h2);
      double a    = double(m_deltaR /  m_h);
      double a2   = Sqr(a);
      double a3   = a2 * a;
      double a4   = Sqr(a2);
      Len2   R2   = Sqr(m_R);
      Len3   R3   = R2 * m_R;
      Len4   R4   = Sqr(R2);
      Len2   r2   = Sqr(m_r);

      // Side Surface Area and Nominal Enclosed Volume:
      Area   sideSurfArea = Pi<double>     * s   * (m_R + m_r);
      Vol    enclVol      = Pi<double>/3.0 * m_h * (R2  + m_R * m_r + r2);

      // "Intrinsic" "empty" MoIs:
      Len4   JE0  =  Pi<double> * h2 * s * (m_r / 2.0  + m_R / 6.0);
      Len4   JE1  =  Pi<double>/4.0  * s * (m_R + m_r) * (R2 + r2);
      Len3   KE   = -Pi<double>/3.0  * s *  m_h * (2.0 * m_r + m_R);

      // Coeffs of "intrtinsic" MoIs with Propellant:
      double JP05 =  Pi<double> / 5.0 * a2;
      Len    JP04 = -Pi<double> / 2.0 * a  * m_R;
      Len2   JP03 =  Pi<double> / 3.0 * R2;

      double JP15 =  Pi<double> /20.0 * a4;
      Len    JP14 = -Pi<double> / 4.0 * a3 * m_R;
      Len2   JP13 =  Pi<double> / 2.0 * a2 * R2;
      Len3   JP12 = -Pi<double> / 2.0 * a  * R3;
      Len4   JP11 =  Pi<double> / 4.0 * R4;

      double KP4  = -Pi<double> / 4.0 * a2;
      Len    KP3  =  Pi<double> * 2.0 / 3.0 * a * m_R;
      Len2   KP2  = -Pi<double> / 2.0 * R2;

      // Initialise the Parent Classes' Flds: NB: For (x0,y0,z0), IsLeft=true:
      RotationBody::Init
        (sideSurfArea, enclVol, a_empty_mass,
         a_alpha,      a_x0,    a_y0,  a_z0,  true, m_h, JE0, JE1, KE,
         a_rho,        LevelOfVol,
         JP05,         JP04,    JP03,
         JP15,         JP14,    JP13,  JP12,  JP11,
         KP4,          KP3,     KP2);
    }

    //=======================================================================//
    // Non-Default Ctor, Simple Cases (TrCone/Cylinder with OX axis):        //
    //=======================================================================//
    // "TrCone" with the rotation axis coinsiding with Ox: y0=z0=alpha=0:
    //
    constexpr TrCone
    (
      Len         a_x0,       // Left  (Upper, Smaller-X) base center
      Len         a_d0,       // Left  (Upper, Smaller-X) base diameter
      Len         a_d1,       // Right (Lower, Larger-X)  base diameter
      Len         a_h,        // Height
      Density     a_rho,      // 0 may be OK
      Mass        a_empty_mass = UnKnownMass
    )
    : TrCone(a_x0, 0.0_m, 0.0_m, 0.0, a_d0, a_d1, a_h, a_rho, a_empty_mass)
    {}

    // As above, but with d0=d1, ie a Cylinder:
    constexpr TrCone
    (
      Len         a_x0,       // Left  (Upper, Smaller-X) base center
      Len         a_d0,       // Left  (Upper, Smaller-X) base diameter
      Len         a_h,        // Height
      Density     a_rho,      // 0 may be OK
      Mass        a_empty_mass = UnKnownMass
    )
    : TrCone(a_x0, 0.0_m, 0.0_m, 0.0, a_d0, a_d0, a_h, a_rho, a_empty_mass)
    {}
  };

  //=========================================================================//
  // "SpherSegm" Class:                                                      //
  //=========================================================================//
  // Similar to "TrCone", but for a Spherical Segment of the base diameter "d"
  // and the height (from the base plane to the pole) "h", where we assume
  // h <= d/2 (the equality corresponds to the case of a HemiSphere).
  // NB: this is a Shperical Segment,  NOT a Spherical Slice,  ie it always
  // contains a pole. The Spherical Slice would be a more general case and a
  // more close analogy of the Truncated Cone, but it would (arguably) have
  // almost no real applications.
  // Furthermore, "alpha" is the angle between the segment's axis and the po-
  // sitive direction of the X axis; we assume that |alpha| < Pi/2;
  // (x0, y0, z0) are the co-ords of the base center (NOT of the pole!), and
  // either "z0" or "y0" must be 0;  in the former case, the Segment's rotat-
  // ion axis is assumed to lie in the XY plane, in the latter -- in XZ.
  // The spherical segment may be facing "right" (ie towards the positive dir-
  // ection of the OX axis), or otherise, as given by the "facing_right" flag.
  // When alpha=0, the axis of the Spherical Segment coincides with OX, which
  // is the most common case:
  //
  class SpherSegm final: public RotationBody
  {
  private:
    //=======================================================================//
    // Data Flds: Spherical Segment's Geometry:                              //
    //=======================================================================//
    // Over-all sizes and Orientation:
    Len    m_r;              // Segment Base Radius
    Len    m_R;              // Sphere  Radius
    Len3   m_R3;             // Radius^3
    bool   m_facingRight;

    // Default Ctor is deleted:
    SpherSegm() = delete;

    //=======================================================================//
    // Propellant Volume -> Propellant Level:                                //
    //=======================================================================//
    constexpr static Len LevelOfVol(Vol a_v, RotationBody const* a_ctx)
    {
      // "a_ctx" must actually be a ptr to "SpherSegm":
      SpherSegm const* segm = static_cast<SpherSegm const*>(a_ctx);
      assert(segm != nullptr);
      return
        segm->m_facingRight
        ? segm->LevelOfVolRight(a_v)
        : segm->LevelOfVolLeft (a_v);
    }

    //-----------------------------------------------------------------------//
    // For the Right-Facing "SpherSegm":                                     //
    //-----------------------------------------------------------------------//
    constexpr Len LevelOfVolRight(Vol a_v) const
    {
      // Solving the equation
      //   x^2*(3-x) = v,
      // where
      //   "x" is the Propellant level relative to "R": x=l/R, 0 <= x <= 1;
      //   "v" is the Volume/(Pi/3*R^3),                       0 <= v <= 2;
      // by using Halley's Method (to avoid dealing with complex roots in the
      // Cardano formula):
      // x=1/2 is a reasonble initial approximation (XXX: We may use "h"  for
      // a more accurate initial point, but this is not required yet):
      //
      double x   = 0.5;
      double tv  = 3.0 * double(a_v / m_R3) / Pi<double>;

      constexpr double Tol = 100.0 * Eps<double>;
      assert(0.0 <= tv && tv < 2.0 + Tol);
      tv = std::min(2.0,  tv);       // Enforce the boundary

      // For safety, restrict the number of iterations:
      constexpr int N = 100;
      int           i = 0;
      for (; i < N; ++i)
      {
        double x2 = Sqr(x);
        double x3 = x2 * x;
        double x4 = Sqr(x2);
        double dx =
          x * (x - 2.0)   * (x3 - 3.0 * x2 + tv) /
          (2.0 * x4 - 8.0 *  x3 + 9.0 * x2 + tv * (1.0 - x));

        // Iterative update:
        x -= dx;

        // Exit condition:
        if (UNLIKELY(Abs(dx) < Tol))
          break;
      }
      // If we got here w/o achieving the required precision, it's an error:
      assert(i < N);

      // If all is fine: To prevent rounding errors, enforce the boundaries:
      x = std::max(std::min(x, 1.0), 0.0);

      // And finally, the level:
      return x * m_R;
    }

    //-----------------------------------------------------------------------//
    // For the Left-Facing "SpherSegm":                                      //
    //-----------------------------------------------------------------------//
    // Using the invariant
    // V_left(l) + V_right(h-l) = EnclVol:
    //
    constexpr Len LevelOfVolLeft(Vol a_v) const
    {
      assert(!IsNeg(a_v) && a_v <= GetEnclVol());
      Len res = GetHeight() - LevelOfVolRight(GetEnclVol() - a_v);
      assert(!IsNeg(res) && res <= GetHeight());
      return res;
    }

  public:
    //=======================================================================//
    // Non-Default Ctor:                                                     //
    //=======================================================================//
    constexpr SpherSegm
    (
      bool        a_facing_right,
      Len         a_x0,          // Base center
      Len         a_y0,          //
      Len         a_z0,          //
      double      a_alpha,       // Dimension-less, in (-Pi/2 .. Pi/2)
      Len         a_d,           // Base diameter
      Len         a_h,           // Height
      Density     a_rho,         // Propellant Density (may be 0)
      Mass        a_empty_mass = UnKnownMass
    )
    {
      //---------------------------------------------------------------------//
      // Over-All Geometry:                                                  //
      //---------------------------------------------------------------------//
      assert(IsPos(a_d)   &&  IsPos(a_h) && Abs(a_alpha) < Pi_2<double> &&
            !IsNeg(a_rho) && !IsNeg(a_empty_mass));

      m_r           = a_d / 2.0;                         // Base   radius
      assert(a_h   <= m_r * (1.0 + 10.0 * Eps<double>)); // Import. constraint!
      m_R           = (Sqr(m_r) / a_h + a_h) / 2.0;      // Sphere radius
      m_R3          = Cube(m_R);
      m_facingRight = a_facing_right;

      //---------------------------------------------------------------------//
      // Parent Classes Initialisation:                                      //
      //---------------------------------------------------------------------//
      // Side Surface Area and Notional Volume:
      Len2 h2     = Sqr(a_h);
      Len3 h3     = a_h * h2;
      Len2 R2     = Sqr(m_R);

      // Side Surface Area and Nominal Enclosed Volume:
      Area sideSurfArea = TwoPi<double> * m_R *  a_h;
      Vol  enclVol      =    Pi<double> * h2  * (m_R - a_h / 3.0);

      // "Intrinsic" "empty" MoIs (NB: they do not depend on "FacingRight",
      // because JE0 is wrt the rotation axis "xi"; furthermore, the Left- and
      // Right-facing segments  are mutually-symmetric wrt any axis  orthogonal
      // to "xi", hence "JE1" must also be the same; for "KE", it is just a lu-
      // cky coincidence due to f(xi)*sqrt(1+f'(xi)^2)=R=const):
      //
      Len4 JE0    = TwoPi<double> / 3.0 * m_R * h3;
      Len4 JE1    = m_R * enclVol;
      Len3 KE     = -  Pi<double> * m_R * h2;

      // Coeffs of "intrtinsic" MoIs with Propellant. Unlike the "empty" ones
      // above, these coeffs do depend on the Segment orientation:
      Len Rmh  = m_R - a_h;
      assert(!IsNeg(Rmh));
      Len TRmh = m_R + Rmh;

      double JP05 = -Pi<double> / 5.0;
      Len    JP04 = (Pi<double> / 2.0)  * (a_facing_right ? m_R : -Rmh);
      Len2   JP03 =  a_facing_right
                     ? Len2(0.0)
                     : Pi<double> / 3.0 * TRmh * a_h;

      double JP15 =  Pi<double> / 20.0;
      Len    JP14 = (Pi<double> / 4.0)  * (a_facing_right ? -m_R : Rmh);

      Len2   JP13 =  Pi<double> *
                     (a_facing_right
                      ? R2 / 3.0
                      : R2 / 3.0 - m_R * a_h + h2 / 2.0);

      Len3   JP12 =  a_facing_right
                     ? Len3(0.0)
                     : (-Pi<double> / 2.0) * Rmh * TRmh * a_h;

      Len4   JP11 =  a_facing_right
                     ? Len4(0.0)
                     : ( Pi<double> / 4.0) * Sqr(TRmh)  * h2;

      double KP4  =  Pi<double> / 4.0;
      Len    KP3  = (Pi<double> * 2.0/3.0) * (a_facing_right  ? -m_R : Rmh);
      Len2   KP2  =  a_facing_right
                     ? Len2(0.0) : (-Pi<double> / 2.0) * TRmh * a_h;

      // Initialise the Parent Classes' Flds:
      // NB: (x0,y0,z0) is the Base Center, so for it, IsLeft = FacingRight:
      RotationBody::Init
        (sideSurfArea, enclVol, a_empty_mass,
         a_alpha,   a_x0, a_y0, a_z0, a_facing_right, a_h, JE0, JE1, KE,
         a_rho,     LevelOfVol,
         JP05,      JP04, JP03,
         JP15,      JP14, JP13, JP12, JP11,
         KP4,       KP3,  KP2);
    }

    //=======================================================================//
    // Non-Default Ctor, Simple Cases:
    //=======================================================================//
    // Rotation axis coinciding with OX:
    //
    constexpr SpherSegm
    (
      bool        a_facing_right,
      Len         a_x0,       // Base center
      Len         a_d,        // Base diameter
      Len         a_h,        // Height
      Density     a_rho,      // 0 may be OK
      Mass        a_empty_mass = UnKnownMass
    )
    : SpherSegm(a_facing_right, a_x0, 0.0_m, 0.0_m, 0.0, a_d, a_h, a_rho,
                a_empty_mass)
    {}

    // As above, but with d/2 = h, ie a HemiSpehere:
    //
    constexpr SpherSegm
    (
      bool        a_facing_right,
      Len         a_x0,       // Base center
      Len         a_d,        // Base diameter
      Density     a_rho,      // 0 may be OK
      Mass        a_empty_mass = UnKnownMass
    )
    : SpherSegm(a_facing_right, a_x0, 0.0_m, 0.0_m, 0.0, a_d, a_d / 2.0, a_rho,
                a_empty_mass)
    {}
  };
}
// End namespace SpaceBallistics
