/***************************************************************************
                             qgsbox3d.cpp
                             ------------
    begin                : April 2017
    copyright            : (C) 2017 by Nyall Dawson
    email                : nyall dot dawson at gmail dot com
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsbox3d.h"
#include "qgspoint.h"
#include "qgslogger.h"

QgsBox3D::QgsBox3D( double xmin, double ymin, double zmin, double xmax, double ymax, double zmax, bool normalize )
  : mBounds2d( xmin, ymin, xmax, ymax, false )
  , mZmin( zmin )
  , mZmax( zmax )
{
  if ( normalize )
  {
    QgsBox3D::normalize();
  }
}

QgsBox3D::QgsBox3D( const QgsPoint &p1, const QgsPoint &p2, bool normalize )
  : mBounds2d( p1.x(), p1.y(), p2.x(), p2.y(), false )
  , mZmin( p1.z() )
  , mZmax( p2.z() )
{
  if ( normalize )
  {
    QgsBox3D::normalize();
  }
}

QgsBox3D::QgsBox3D( const QgsRectangle &rect, double zMin, double zMax, bool normalize )
  : mBounds2d( rect )
  , mZmin( zMin )
  , mZmax( zMax )
{
  if ( normalize )
  {
    QgsBox3D::normalize();
  }
}

void QgsBox3D::setXMinimum( double x )
{
  mBounds2d.setXMinimum( x );
}

void QgsBox3D::setXMaximum( double x )
{
  mBounds2d.setXMaximum( x );
}

void QgsBox3D::setYMinimum( double y )
{
  mBounds2d.setYMinimum( y );
}

void QgsBox3D::setYMaximum( double y )
{
  mBounds2d.setYMaximum( y );
}

void QgsBox3D::setZMinimum( double z )
{
  mZmin = z;
}

void QgsBox3D::setZMaximum( double z )
{
  mZmax = z;
}

/**
 * Set a rectangle so that min corner is at max
 * and max corner is at min. It is NOT normalized.
 */
void QgsBox3D::setMinimal()
{
  mBounds2d.setMinimal();
  mZmin = std::numeric_limits<double>::max();
  mZmax = -std::numeric_limits<double>::max();
}

void QgsBox3D::normalize()
{
  mBounds2d.normalize();
  const double minTmp = std::min( mZmin, mZmax );
  mZmax = std::max( mZmin, mZmax );
  mZmin = minTmp;
}

QgsBox3D QgsBox3D::intersect( const QgsBox3D &other ) const
{
  const QgsRectangle intersect2d = mBounds2d.intersect( other.mBounds2d );
  const double zMin = std::max( mZmin, other.mZmin );
  const double zMax = std::min( mZmax, other.mZmax );
  return QgsBox3D( intersect2d.xMinimum(), intersect2d.yMinimum(), zMin,
                   intersect2d.xMaximum(), intersect2d.yMaximum(), zMax );
}

bool QgsBox3D::is2d() const
{
  return qgsDoubleNear( mZmin, mZmax ) || ( mZmin > mZmax ) || std::isnan( mZmin ) || std::isnan( mZmax ) ;
}

bool QgsBox3D::is3D() const
{
  return !is2d() && !isNull();
}

bool QgsBox3D::intersects( const QgsBox3D &other ) const
{
  if ( !mBounds2d.intersects( other.mBounds2d ) )
    return false;

  if ( other.is2d() || is2d() )
  {
    return true;
  }
  else
  {
    const double z1 = ( mZmin > other.mZmin ? mZmin : other.mZmin );
    const double z2 = ( mZmax < other.mZmax ? mZmax : other.mZmax );
    return z1 <= z2;
  }
}

bool QgsBox3D::contains( const QgsBox3D &other ) const
{
  if ( !mBounds2d.contains( other.mBounds2d ) )
    return false;

  if ( other.is2d() || is2d() )
  {
    return true;
  }
  else
  {
    return ( other.mZmin >= mZmin && other.mZmax <= mZmax );
  }
}

bool QgsBox3D::contains( const QgsPoint &p ) const
{
  if ( is3D() )
  {
    return contains( p.x(), p.y(), p.z() );
  }
  else
  {
    return mBounds2d.contains( p );
  }
}

bool QgsBox3D::contains( double x, double y, double z ) const
{
  if ( !mBounds2d.contains( x, y ) )
  {
    return false;
  }

  if ( std::isnan( z ) || is2d() )
  {
    return true;
  }
  else
  {
    return mZmin <= z && z <= mZmax;
  }
}

/**
 * Expands the bbox so that it covers both the original rectangle and the given rectangle.
 */
void QgsBox3D::combineWith( const QgsBox3D &box )
{
  mBounds2d.combineExtentWith( box.mBounds2d );
  mZmin = std::min( mZmin, box.zMinimum() );
  mZmax = std::max( mZmax, box.zMaximum() );
}

/**
 * Expands the bbox so that it covers both the original rectangle and the given point.
 */
void QgsBox3D::combineWith( double x, double y, double z )
{
  mBounds2d.combineExtentWith( x, y );
  mZmin = std::min( mZmin, z );
  mZmax = std::max( mZmax, z );
}

double QgsBox3D::distanceTo( const  QVector3D &point ) const
{
  const double dx = std::max( mBounds2d.xMinimum() - point.x(), std::max( 0., point.x() - mBounds2d.xMaximum() ) );
  const double dy = std::max( mBounds2d.yMinimum() - point.y(), std::max( 0., point.y() - mBounds2d.yMaximum() ) );
  if ( is2d() || std::isnan( point.z() ) )
  {
    return std::hypot( dx, dy );
  }
  else
  {
    const double dz = std::max( mZmin - point.z(), std::max( 0., point.z() - mZmax ) );
    return std::hypot( dx, dy, dz );
  }
}

bool QgsBox3D::operator==( const QgsBox3D &other ) const
{
  return mBounds2d == other.mBounds2d &&
         qgsDoubleNear( mZmin, other.mZmin ) &&
         qgsDoubleNear( mZmax, other.mZmax );
}

void QgsBox3D::scale( double scaleFactor, const QgsPoint &center )
{
  // scale from the center
  double centerX, centerY, centerZ;
  if ( !center.isEmpty() )
  {
    centerX = center.x();
    centerY = center.y();
    centerZ = center.z();
  }
  else
  {
    centerX = ( xMinimum() + xMaximum() ) / 2;
    centerY = ( yMinimum() + yMaximum() ) / 2;
    centerZ = ( zMinimum() + zMaximum() ) / 2;
  }
  scale( scaleFactor, centerX, centerY, centerZ );
}

void QgsBox3D::scale( double scaleFactor, double centerX, double centerY, double centerZ )
{
  setXMinimum( centerX + ( xMinimum() - centerX ) * scaleFactor );
  setXMaximum( centerX + ( xMaximum() - centerX ) * scaleFactor );

  setYMinimum( centerY + ( yMinimum() - centerY ) * scaleFactor );
  setYMaximum( centerY + ( yMaximum() - centerY ) * scaleFactor );

  setZMinimum( centerZ + ( zMinimum() - centerZ ) * scaleFactor );
  setZMaximum( centerZ + ( zMaximum() - centerZ ) * scaleFactor );
}

bool QgsBox3D::isNull() const
{
  return ( std::isnan( mBounds2d.xMinimum() ) && std::isnan( mBounds2d.xMaximum() )
           && std::isnan( mBounds2d.yMinimum() ) && std::isnan( mBounds2d.xMaximum() )
           && std::isnan( mZmin ) && std::isnan( mZmax ) )
         ||
         ( mBounds2d.xMinimum() == std::numeric_limits<double>::max() && mBounds2d.yMinimum() == std::numeric_limits<double>::max() && mZmin == std::numeric_limits<double>::max()
           && mBounds2d.xMaximum() == -std::numeric_limits<double>::max() && mBounds2d.yMaximum() == -std::numeric_limits<double>::max() && mZmax == -std::numeric_limits<double>::max() );
}

bool QgsBox3D::isEmpty() const
{
  return mZmax < mZmin  || qgsDoubleNear( mZmax, mZmin ) || mBounds2d.isEmpty();
}

QString QgsBox3D::toString( int precision ) const
{
  QString rep;

  if ( precision < 0 )
  {
    precision = 0;
    if ( ( width() < 10 || height() < 10 ) && ( width() > 0 && height() > 0 ) )
    {
      precision = static_cast<int>( std::ceil( -1.0 * std::log10( std::min( width(), height() ) ) ) ) + 1;
      // sanity check
      if ( precision > 20 )
        precision = 20;
    }
  }

  if ( isNull() )
    rep = QStringLiteral( "Null" );
  else if ( isEmpty() )
    rep = QStringLiteral( "Empty" );
  else
    rep = QStringLiteral( "%1,%2,%3 : %4,%5,%6" )
          .arg( mBounds2d.xMinimum(), 0, 'f', precision )
          .arg( mBounds2d.yMinimum(), 0, 'f', precision )
          .arg( mZmin, 0, 'f', precision )
          .arg( mBounds2d.xMaximum(), 0, 'f', precision )
          .arg( mBounds2d.yMaximum(), 0, 'f', precision )
          .arg( mZmax, 0, 'f', precision );

  QgsDebugMsgLevel( QStringLiteral( "Extents : %1" ).arg( rep ), 4 );

  return rep;
}
