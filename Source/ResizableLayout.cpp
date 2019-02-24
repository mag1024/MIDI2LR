/*******************************************************************************
"A Collection of Useful C++ Classes for Digital Signal Processing"
By Vinnie Falco
Official project location:
https://github.com/vinniefalco/DSPFilters
See Documentation.cpp for contact information, notes, and bibliography.
--------------------------------------------------------------------------------
License: MIT License (http://www.opensource.org/licenses/mit-license.php)
Copyright (c) 2009 by Vinnie Falco
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*******************************************************************************/

#include "JuceHeader.h"
#include "ResizableLayout.h"

ResizableLayout::Anchor::Anchor(juce::Component* component_)
    : component(component_), child(dynamic_cast<ResizableChild*>(component_))
{
   jassert(component);
}

bool ResizableLayout::Anchor::operator==(const Anchor& rhs) const noexcept
{
   return component == rhs.component;
}

bool ResizableLayout::Anchor::operator>=(const Anchor& rhs) const noexcept
{
   return component >= rhs.component;
}

bool ResizableLayout::Anchor::operator<(const Anchor& rhs) const noexcept
{
   return component < rhs.component;
}

ResizableLayout::State::State(juce::Component* component_) noexcept : component(component_)
{
   jassert(component);
}

ResizableLayout::State::State(const State& other) noexcept
    : component(other.component), aspect(other.aspect), margin(other.margin)
{
}

ResizableLayout::State& ResizableLayout::State::operator=(const State& other) noexcept
{ // added 20 May 2017 RSJ
   component = other.component;
   aspect = other.aspect;
   margin = other.margin;
   return *this;
}

bool ResizableLayout::State::operator==(const State& rhs) const noexcept
{
   return component == rhs.component;
}

bool ResizableLayout::State::operator>=(const State& rhs) const noexcept
{
   return component >= rhs.component;
}

bool ResizableLayout::State::operator<(const State& rhs) const noexcept
{
   return component < rhs.component;
}

//------------------------------------------------------------------------------

ResizableLayout::ResizableLayout(juce::Component* owner) : m_owner(owner)
{
   m_bFirstTime = true;
   m_isActive = false;

   m_owner->addComponentListener(this);
}

ResizableLayout::~ResizableLayout() {}

#pragma warning(suppress : 26429)
void ResizableLayout::addToLayout(juce::Component* component, const juce::Point<int>& topLeft,
    const juce::Point<int>& bottomRight, Style style)
{
   jassert(topLeft != anchorNone);

   Anchor anchor(component);
   anchor.style = style;
   anchor.topLeft = topLeft;
   if (bottomRight == anchorNone)
      anchor.bottomRight = topLeft;
   else
      anchor.bottomRight = bottomRight;

#pragma warning(suppress : 26486)
   m_anchors.add(anchor);

   component->addComponentListener(this);
}

void ResizableLayout::removeFromLayout(juce::Component* component) noexcept
{
   m_anchors.removeValue(component);
   m_states.removeValue(component);
}

void ResizableLayout::activateLayout(bool isActive) noexcept
{
   if (m_isActive != isActive) {
      if (isActive && m_bFirstTime) {
         updateLayout();
         m_bFirstTime = false;
      }

      m_isActive = isActive;
   }
}

void ResizableLayout::updateLayout() noexcept
{
   m_states.clearQuick();
   for (int i = 0; i < m_anchors.size(); i++)
#pragma warning(suppress : 26446)
      addStateFor(m_anchors[i]);
}

void ResizableLayout::updateLayoutFor(juce::Component* component) noexcept
{
   m_states.removeValue(component);
#pragma warning(suppress : 26446)
   addStateFor(m_anchors[m_anchors.indexOf(component)]);
}

void ResizableLayout::addStateFor(const Anchor& anchor) noexcept
{
#pragma warning(suppress : 26496)
   Rect rb = anchor.component->getBounds();

   const auto w = m_owner->getWidth();
   const auto h = m_owner->getHeight();

   State state(anchor.component);

   // secret sauce
   state.margin.top = rb.top - (h * anchor.topLeft.getY()) / anchorUnit;
   state.margin.left = rb.left - (w * anchor.topLeft.getX()) / anchorUnit;
   state.margin.bottom = rb.bottom - (h * anchor.bottomRight.getY()) / anchorUnit;
   state.margin.right = rb.right - (w * anchor.bottomRight.getX()) / anchorUnit;

   state.aspect = double(rb.getWidth()) / rb.getHeight();

#pragma warning(suppress : 26486)
   m_states.add(state);
}

void ResizableLayout::recalculateLayout() const
{
   if (m_isActive) {
#pragma warning(suppress : 26496)
      Rect rp = m_owner->getBounds();

      for (int i = 0; i < m_states.size(); i++) {
#pragma warning(suppress : 26446)
         auto anchor = m_anchors[i];
#pragma warning(suppress : 26446)
         const auto state = m_states[i];
         jassert(anchor.component == state.component);

         Rect rb;

         // secret sauce
         rb.top = state.margin.top + (rp.getHeight() * anchor.topLeft.getY()) / anchorUnit;
         rb.left = state.margin.left + (rp.getWidth() * anchor.topLeft.getX()) / anchorUnit;
         rb.bottom =
             state.margin.bottom + (rp.getHeight() * anchor.bottomRight.getY()) / anchorUnit;
         rb.right = state.margin.right + (rp.getWidth() * anchor.bottomRight.getX()) / anchorUnit;
         // prove that there are no rounding issues
         jassert((anchor.bottomRight != anchor.topLeft)
                 || ((rb.getWidth() == anchor.component->getWidth())
                        && (rb.getHeight() == anchor.component->getHeight())));

         if (anchor.style == styleStretch) {
            anchor.component->setBounds(rb);
         }
         else if (anchor.style == styleFixedAspect) {
            Rect r;
            const auto aspect = double(rb.getWidth()) / rb.getHeight();

            if (aspect > state.aspect) {
               r.top = rb.top;
               r.bottom = rb.bottom;
               const auto width = int(state.aspect * r.getHeight());
               r.left = rb.left + (rb.getWidth() - width) / 2;
               r.right = r.left + width;
            }
            else {
               r.left = rb.left;
               r.right = rb.right;
               const auto height = int(1. / state.aspect * r.getWidth());
               r.top = rb.top + (rb.getHeight() - height) / 2;
               r.bottom = r.top + height;
            }

            anchor.component->setBounds(r);
         }
      }
   }
}

void ResizableLayout::componentMovedOrResized(
    juce::Component& component, bool /*wasMoved*/, bool wasResized)
{
   if (&component == m_owner) {
      if (wasResized) {
         recalculateLayout();
      }
   }
   else {
      updateLayoutFor(&component);
   }
}

void ResizableLayout::componentBeingDeleted(juce::Component& component) noexcept
{
   if (&component != m_owner) {
      m_anchors.removeValue(&component);
      m_states.removeValue(&component);
   }
}

#pragma warning(suppress : 26429 26461)
juce::Rectangle<int> ResizableLayout::calcBoundsOfChildren(juce::Component* parent) noexcept
{
   juce::Rectangle<int> r;

   for (int i = 0; i < parent->getNumChildComponents(); i++)
      r = r.getUnion(parent->getChildComponent(i)->getBounds());
   return r;
}

void ResizableLayout::resizeStart() noexcept
{
   auto haveChildren = false;

   // first recursively call resizeStart() on all children
   for (int i = 0; i < m_anchors.size(); i++) {
      auto child = m_anchors.getUnchecked(i).child;
      if (child) {
         child->resizeStart();
         haveChildren = true;
      }
   }

   // now check our immediate children for constraints
   if (haveChildren) {
      // start with our values
      auto xmin0 = getMinimumWidth();
      auto ymin0 = getMinimumHeight();

      // for each child, use the inverse of the resize function to solve for
      // the min/max
      for (int i = 0; i < m_anchors.size(); i++) {
         auto anchor = m_anchors.getUnchecked(i);
         if (anchor.child) {
            const auto state = m_states.getUnchecked(i);
            jassert(anchor.component == state.component);

            const auto xmin1 = anchor.child->getMinimumWidth();
            const auto ymin1 = anchor.child->getMinimumHeight();
            // int xmax1 = anchor.child->getMaximumWidth();
            // int ymax1 = anchor.child->getMaximumHeight();

            int m; // solution to f'(x)

            auto d = anchor.bottomRight.getX() - anchor.topLeft.getX();
            if (d != 0) {
               m = (xmin1 + state.margin.left - state.margin.right) * anchorUnit / d;
               xmin0 = juce::jmax(xmin0, m);
            }

            d = anchor.bottomRight.getY() - anchor.topLeft.getY();
            if (d != 0) {
               m = (ymin1 + state.margin.top - state.margin.bottom) * anchorUnit / d;
               ymin0 = juce::jmax(ymin0, m);
            }
         }
      }

      // apply adjusted limits
      setMinimumWidth(xmin0);
      setMinimumHeight(ymin0);
   }
}

//------------------------------------------------------------------------------

TopLevelResizableLayout::Constrainer::Constrainer(TopLevelResizableLayout* owner) noexcept
    : m_owner(*owner)
{
}

void TopLevelResizableLayout::Constrainer::resizeStart() noexcept
{
   m_owner.resizeStart();
   const auto minWtmp = m_owner.getMinimumWidth();
   const auto minHtmp = m_owner.getMinimumHeight();
   setMinimumWidth(minWtmp);
   setMinimumHeight(minHtmp);
}

//------------------------------------------------------------------------------

TopLevelResizableLayout::TopLevelResizableLayout(juce::Component* owner)
    : ResizableLayout(owner), m_constrainer(this)
{
}

#pragma warning(suppress : 26429)
void TopLevelResizableLayout::setAsConstrainerFor(juce::ResizableWindow* window)
{
   window->setConstrainer(&m_constrainer);
}