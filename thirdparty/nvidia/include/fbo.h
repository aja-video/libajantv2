/*
 * Copyright 1993-2012 NVIDIA Corporation.  All rights reserved.
 *
 * NOTICE TO LICENSEE:
 *
 * This source code and/or documentation ("Licensed Deliverables") are
 * subject to NVIDIA intellectual property rights under U.S. and
 * international Copyright laws.
 *
 * These Licensed Deliverables contained herein is PROPRIETARY and
 * CONFIDENTIAL to NVIDIA and is being provided under the terms and
 * conditions of a form of NVIDIA software license agreement by and
 * between NVIDIA and Licensee ("License Agreement") or electronically
 * accepted by Licensee.  Notwithstanding any terms or conditions to
 * the contrary in the License Agreement, reproduction or disclosure
 * of the Licensed Deliverables to any third party without the express
 * written consent of NVIDIA is prohibited.
 *
 * NOTWITHSTANDING ANY TERMS OR CONDITIONS TO THE CONTRARY IN THE
 * LICENSE AGREEMENT, NVIDIA MAKES NO REPRESENTATION ABOUT THE
 * SUITABILITY OF THESE LICENSED DELIVERABLES FOR ANY PURPOSE.  IT IS
 * PROVIDED "AS IS" WITHOUT EXPRESS OR IMPLIED WARRANTY OF ANY KIND.
 * NVIDIA DISCLAIMS ALL WARRANTIES WITH REGARD TO THESE LICENSED
 * DELIVERABLES, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY,
 * NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE.
 * NOTWITHSTANDING ANY TERMS OR CONDITIONS TO THE CONTRARY IN THE
 * LICENSE AGREEMENT, IN NO EVENT SHALL NVIDIA BE LIABLE FOR ANY
 * SPECIAL, INDIRECT, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, OR ANY
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THESE LICENSED DELIVERABLES.
 *
 * U.S. Government End Users.  These Licensed Deliverables are a
 * "commercial item" as that term is defined at 48 C.F.R. 2.101 (OCT
 * 1995), consisting of "commercial computer software" and "commercial
 * computer software documentation" as such terms are used in 48
 * C.F.R. 12.212 (SEPT 1995) and is provided to the U.S. Government
 * only as a commercial end item.  Consistent with 48 C.F.R.12.212 and
 * 48 C.F.R. 227.7202-1 through 227.7202-4 (JUNE 1995), all
 * U.S. Government End Users acquire the Licensed Deliverables with
 * only those rights set forth herein.
 *
 * Any use of the Licensed Deliverables in individual and commercial
 * software must include, in the user documentation and internal
 * comments to the code, the above Disclaimer and U.S. Government End
 * Users Notice.
 */
#ifndef _CFBO
#define _CFBO


// Useful Macros

class CFBO
{
    public:
        CFBO() 
          : fboId(0), numRenderbuffers(0), valid(GL_FALSE)
	
        { 
		}


		void status()
		{
			GLenum status;
			status = (GLenum) glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
			switch(status) {
				case GL_FRAMEBUFFER_COMPLETE_EXT:
					break;
				case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
					printf("Unsupported framebuffer format\n");
					break;
				case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
					printf("Framebuffer incomplete, incomplete attachment\n");
					break;
				case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
					printf("Framebuffer incomplete, missing attachment\n");
					break;
				case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
					printf("Framebuffer incomplete, attached images must have same dimensions\n");
					break;
				case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
					printf("Framebuffer incomplete, attached images must have same format\n");
					break;
				case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
					printf("Framebuffer incomplete, missing draw buffer\n");
					break;
				case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
					printf("Framebuffer incomplete, missing read buffer\n");
					break;
				default:
					;
					//assert(0);
			}
		}

          bool create(int width, int height,
                    int sizeBits,             // bits per component
					int num_samples,		  // number of samples
                    GLboolean alpha,          // alpha
                    GLboolean depth,		  // depth
					GLuint textureObject)	  // texture object          

        {
			GLenum texFormat;

			// Clean up fbo resources if already created    
			if (valid)
                destroy();

            switch (sizeBits) {
            case 8:
                if (alpha) {
                    texFormat = GL_RGBA8;
                } else {
                    texFormat = GL_RGB8;
                }
                break;

            case 10:
                if (alpha) {
                    texFormat = GL_RGB10_A2UI;
                } else {
                    texFormat = GL_RGB10_A2UI;
                }
                break;

			case 16:
                if (alpha) {
                    texFormat = GL_RGBA16F_ARB;
                } else {
                    texFormat = GL_RGB16F_ARB;
                }
                break;

            default:
                return GL_FALSE;
            }

			// Generate FBO
			//uint32_t a = (uint32_t)&glGenFramebuffersEXT;
            glGenFramebuffersEXT(1, &fboId);

			// Bind FBO
            glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fboId);

			// Calculate the number of required render buffers.
            numRenderbuffers = 1;
            if (depth) {
                numRenderbuffers++;
            }

			// Generate required render buffers.
            glGenRenderbuffersEXT(numRenderbuffers, renderbufferIds);

			// Bind color render buffer.
            glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, renderbufferIds[0]);

			// Allocate storage for color render buffer
			if ((!textureObject) && (num_samples > 1)) {
				glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER_EXT, num_samples, texFormat, width, height);
			} else if (!textureObject){
				glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, texFormat, width, height);
			}

			// Get number of samples from allocated color render buffer
			glGetRenderbufferParameterivEXT(GL_RENDERBUFFER_EXT, GL_RENDERBUFFER_SAMPLES_EXT, &num_samples);

			// Attach color render buffer.
			if (!textureObject) {
				glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,
					                         GL_COLOR_ATTACHMENT0_EXT,
						                     GL_RENDERBUFFER_EXT,
							                 renderbufferIds[0]);
			} else {
				glBindTexture(GL_TEXTURE_2D, textureObject);
				glFramebufferTexture2DEXT( GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, 
					                       GL_TEXTURE_2D, textureObject, 0 );
			}

			status();

			// Bind and allocate storage for depth render buffer
            if (depth) {
				glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, renderbufferIds[1]);
				if (num_samples > 1) {
					glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER_EXT, num_samples, GL_DEPTH_COMPONENT, width, height);
				} else {
					glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT, width, height);
				}
			}

			// Attached depth render buffer.
            if (depth) {
                glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,
                                             GL_DEPTH_ATTACHMENT_EXT,
                                             GL_RENDERBUFFER_EXT,
                                             renderbufferIds[1]);
            }
   
			status();

            glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);
            glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

            if (glGetError() == GL_NO_ERROR) {
                valid = GL_TRUE;
				return GL_TRUE;
            } else {
                destroy();
			}

            return GL_FALSE;
     
		}

        void bind(int width, int height)
        {
            if (!valid) {
                glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
            } else {
                glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,
                                     fboId);
            }

            glViewport(0, 0, width, height);
        }

        void bindRead(int width, int height)
        {
            if (!valid) {
                glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, 0);
            } else {
                glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT,
                                     fboId);
            }

            glViewport(0, 0, width, height);
        }

		void bindDraw(int width, int height)
        {
            if (!valid) {
                glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, 0);
            } else {
                glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT,
                                     fboId);
            }

            glViewport(0, 0, width, height);
        }

		void unbind()
		{
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
		}


		void destroy()
        {
            if (valid)
            {
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
            glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);

            glDeleteFramebuffersEXT(1, &fboId);

            glDeleteRenderbuffersEXT(numRenderbuffers, renderbufferIds);

			valid = GL_FALSE;

			}
		}

		~CFBO()
        {
            destroy();
        }
		
//	private:
		GLuint fboId;

        // Max two renderbuffers per FBO:
        //   - One colorbuffer
        // - One [optional] depthbuffer
        GLuint renderbufferIds[2];

        GLint numRenderbuffers;
        GLboolean valid;

};
#endif